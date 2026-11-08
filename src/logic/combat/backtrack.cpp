// Backtrack - hit your target where it was a moment ago.
//
// While you fight a target in range, the movement packets the server sends about that
// target are held back for up to "delay" ms. On your screen the target stays where it was,
// which makes it easier to hit when it moves away. Packets are released once the hold time
// is up, when the target would be closer if released, or when you take knockback
// ("disable_on_hit"). A box can show where the target really is.
//
// Callbacks:
//   backtrack_packet_hook_under    every incoming packet (network thread): hold or release
//   backtrack_render_box_held      every frame: draw the box at the target's real position
//   backtrack_tick_decide_whether  every tick: start / keep / stop backtracking
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };

struct RenderHook {
    char padding_00[8];
    Vec3 cam;           // +0x08 camera position
    char padding_20[8];
    float partial;      // +0x28 how far we are between two ticks (0..1)
};

struct PktHook {
    char padding_00[8];
    void *ctx;              // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    long long budget;       // +0x18 ms the packet has been held so far
    int seq;                // +0x20 packet sequence number
    int verdict;            // +0x24 0 = let through, 1 = hold, 3 = let through even while holding
};

// ---- settings ----
extern "C" float backtrack_delay;                             // "delay": max hold (ms)
extern "C" float backtrack_cooldown;                          // "cooldown": ticks to wait after stopping
extern "C" float backtrack_max_target_hurt_time;              // only while the target's hurt time is at most this
extern "C" float backtrack_min_max_range, backtrack_min_max_range_second;   // target distance range (min, max)
extern "C" unsigned char backtrack_require_mouse_pressed_value;   // "require_mouse_pressed"
extern "C" unsigned char backtrack_require_weapon_value;          // "require_weapon"
extern "C" unsigned char backtrack_disable_on_hit_value;          // "disable_on_hit": stop when we take knockback
extern "C" unsigned char backtrack_render_remote_pos;             // "render_remote_pos": draw the box
extern "C" unsigned char backtrack_rotate_box;                    // turn the box with the target's yaw
extern "C" float backtrack_line_width_setting;                    // "line_width"
extern "C" float backtrack_color_setting[3];                      // "color" (r, g, b)
extern "C" unsigned char backtrack_filled_setting;                // "filled"

// ---- module state (shared with the network thread, guarded by the spin lock) ----
extern "C" unsigned char backtrack_spin_lock;
extern "C" unsigned char backtrack_active;                 // holding the target's packets
extern "C" long long backtrack_max_hold_budget;            // "delay" as a whole number
extern "C" int backtrack_target_entity_id;
extern "C" int backtrack_own_entity_id;
extern "C" unsigned char backtrack_stop_on_velocity_setting;   // copy of "disable_on_hit" for the network thread
extern "C" int backtrack_last_released_sequence;
extern "C" int backtrack_last_held_sequence;
extern "C" int backtrack_cooldown_ticks;
// The target's real (server-side) position, built from the held packets (see reach_distance_checks.cpp):
extern "C" Vec3 backtrack_accumulated_offset;                       // movement in the held packets (or an absolute position, see below)
extern "C" unsigned char backtrack_stop_on_velocity_setting_field_0x1f;  // 1: the offset is an absolute position (a teleport was held)
extern "C" float backtrack_stop_on_velocity_setting_field_0x23;          // yaw from the held packets
extern "C" unsigned char backtrack_stop_on_velocity_setting_field_0x27;  // 1: that yaw is set
// For the box: real position and yaw, last tick / this tick / latest.
extern "C" unsigned char backtrack_render_box;
extern "C" Vec3 backtrack_previous_current_server_position;                              // last tick
extern "C" Vec3 backtrack_previous_current_server_position_in_reach_distance_checks_a;   // this tick
extern "C" Vec3 backtrack_previous_current_server_position_in_reach_distance_checks;     // latest
extern "C" float backtrack_yaws, backtrack_yaws_in_reach_distance_checks_a, backtrack_yaws_in_reach_distance_checks;

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlScalef)(float, float, float);
extern "C" void (**ptr_to_pGlRotatef)(float, float, float, float);
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);
FN(void, gl_array_xyz_float, ());          // set up the unit box vertex array
FN(void, gl_array_color1, ());             // draw it filled
FN(void, gl_array_color2, ());             // draw its outline
FN(void, disable_vertex_array_state, ());

// ---- game / engine functions ----
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                       // a mouse button is held
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));   // position the client shows
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));
FN(unsigned char, entity_reach_check, (Ctx *ctx, void *entity, Vec3 eye));                 // worth starting? (see reach_distance_checks.cpp)
FN(double, segment_distance, (Vec3 eye, Vec3 pos, Vec3 off));                              // how much further away the real position is
FN(void, backtrack_record_server_position, (Vec3 off, float yaw));
FN(void *, java_jni_is_instance_of_s19_packet_entity_status, (void *ctx, void *event));
FN(int, java_get_s19_packet_entity_status_entity_id, (void *ctx, void *s19_packet_entity_status));
FN(void *, java_jni_is_instance_of_s06_packet_update_health, (void *ctx, void *event));
FN(void *, java_jni_is_instance_of_s29_packet_sound_effect, (void *ctx, void *event));
FN(void *, java_jni_is_instance_of_s12_packet_entity_velocity, (void *ctx, void *event));
FN(int, java_get_s12_packet_entity_velocity_entity_id, (void *ctx, void *s12_packet_entity_velocity));
FN(void *, java_jni_is_instance_of_s08_packet_player_pos_look, (void *ctx, void *event));  // server teleported us
FN(void, check_entity_distance, (void **hook_ctx, unsigned char hold));                     // track the target's movement in a packet

static void lock() {
    while (__atomic_exchange_n(&backtrack_spin_lock, (unsigned char)1, __ATOMIC_SEQ_CST))
        __builtin_ia32_pause();
}
static void unlock() { backtrack_spin_lock = 0; }

// Forget the tracked movement.
static void clear_tracked_offset() {
    backtrack_accumulated_offset = vec3_make(0.0, 0.0, 0.0);
    backtrack_stop_on_velocity_setting_field_0x1f = 0;
    backtrack_stop_on_velocity_setting_field_0x27 = 0;
}

// Every incoming packet (network thread).
extern "C" void backtrack_packet_hook_under(PktHook *hook) {
    lock();
    if (backtrack_active == 1) {
        // Hurt / health / sound packets about the target go through at once (hit feedback).
        bool pass = false;
        void *status = java_jni_is_instance_of_s19_packet_entity_status(hook->ctx, hook->ev);
        if (status)
            pass = java_get_s19_packet_entity_status_entity_id(hook->ctx, status) == backtrack_target_entity_id;
        else
            pass = java_jni_is_instance_of_s06_packet_update_health(hook->ctx, hook->ev) || java_jni_is_instance_of_s29_packet_sound_effect(hook->ctx, hook->ev);
        if (pass) {
            hook->verdict = 3;
            unlock();
            return;
        }

        // We were teleported, or took knockback with "disable_on_hit": stop.
        bool stop = false;
        void *velocity = java_jni_is_instance_of_s12_packet_entity_velocity(hook->ctx, hook->ev);
        if (!velocity)
            stop = java_jni_is_instance_of_s08_packet_player_pos_look(hook->ctx, hook->ev) != nullptr;
        else
            stop = backtrack_stop_on_velocity_setting && java_get_s12_packet_entity_velocity_entity_id(hook->ctx, velocity) == backtrack_own_entity_id;
        if (stop) {
            backtrack_active = 0;
            unlock();
            return;
        }

        if (hook->budget <= backtrack_max_hold_budget) {
            // Hold it, and add its movement to the tracked offset (once per packet).
            hook->verdict = 1;
            int seq = hook->seq;
            if (seq > backtrack_last_released_sequence) {
                backtrack_last_released_sequence = seq;
                check_entity_distance(&hook->ctx, 0);
            }
        } else if (hook->verdict == 0 && hook->seq > backtrack_last_held_sequence) {
            // Held long enough: released now, take its movement back out of the offset.
            check_entity_distance(&hook->ctx, 1);
            backtrack_last_held_sequence = hook->seq;
        }
    }
    unlock();
}

// Every frame: a box (0.6 wide, 2 tall) at the target's real position.
extern "C" void backtrack_render_box_held(RenderHook *hook) {
    if (backtrack_render_box != 1)
        return;
    Vec3 pos = vec3_sub(vec3_lerp(backtrack_previous_current_server_position, backtrack_previous_current_server_position_in_reach_distance_checks_a, (double)hook->partial), hook->cam);
    gl_array_xyz_float();
    (*ptr_to_pGlLineWidth)(backtrack_line_width_setting);
    (*ptr_to_pGlPushMatrix)();
    (*ptr_to_pGlTranslatef)((float)pos.x, (float)(pos.y + 1.0), (float)pos.z);
    (*ptr_to_pGlScalef)(0.3f, 1.0f, 0.3f);
    if (backtrack_rotate_box == 1) {
        float yaw = lerp_unclamped_float(backtrack_yaws, backtrack_yaws_in_reach_distance_checks_a, hook->partial);
        (*ptr_to_pGlRotatef)(180.0f - yaw, 0.0f, 1.0f, 0.0f);
    }
    if (backtrack_filled_setting == 1) {
        (*ptr_to_pGlColor4f)(backtrack_color_setting[0], backtrack_color_setting[1], backtrack_color_setting[2], 0.4f);
        gl_array_color1();
    }
    (*ptr_to_pGlColor3f)(backtrack_color_setting[0], backtrack_color_setting[1], backtrack_color_setting[2]);
    gl_array_color2();
    (*ptr_to_pGlPopMatrix)();
    disable_vertex_array_state();
}

// Are we fighting a target that is in range?
static bool should_backtrack(Ctx *ctx) {
    if (ctx->in_game != 1)
        return false;
    if (backtrack_require_mouse_pressed_value == 1 && !vertex_pointer_current(ctx))
        return false;
    if (backtrack_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return false;
    // We attacked in the last second.
    int last_attack = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
    if (java_get_entity_ticks_existed(ctx->jvm, ctx->player) - last_attack > 20)
        return false;
    CombatTarget *target = (CombatTarget *)ctx->target;
    if (!target->ent)
        return false;
    if ((float)java_get_entity_living_base_hurt_time(ctx->jvm, target->ent) > backtrack_max_target_hurt_time)
        return false;
    double distance_sq = target->dist2;
    if ((double)(backtrack_min_max_range * backtrack_min_max_range) > distance_sq ||
        distance_sq > (double)(backtrack_min_max_range_second * backtrack_min_max_range_second))
        return false;
    return true;
}

// Every tick: start, keep or stop backtracking.
extern "C" void backtrack_tick_decide_whether(Hook *hook) {
    Ctx *ctx = hook->ctx;
    int cooldown = backtrack_cooldown_ticks;
    if (cooldown > 0) {
        backtrack_cooldown_ticks = cooldown - 1;
        if (cooldown != 1)
            return;
    }

    if (!should_backtrack(ctx)) {
        lock();
        if (backtrack_active == 1) {
            backtrack_cooldown_ticks = (int)backtrack_cooldown;
            backtrack_active = 0;
        }
        unlock();
        backtrack_render_box = 0;
        return;
    }

    void *target = ((CombatTarget *)ctx->target)->ent;
    Vec3 eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (java_variant(ctx->jvm))
        eye = vec3_add_eye_height_sneak(eye);
    Vec3 shown_pos = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, target);
    float yaw = get_entity_rotation(ctx->jvm, target).yaw;
    int target_id = java_get_entity_entity_id(ctx->jvm, target);
    int own_id = java_get_entity_entity_id(ctx->jvm, ctx->player);

    lock();
    backtrack_max_hold_budget = (long long)backtrack_delay;
    int old_target_id = backtrack_target_entity_id;
    backtrack_target_entity_id = target_id;
    backtrack_own_entity_id = own_id;
    backtrack_stop_on_velocity_setting = backtrack_disable_on_hit_value;

    if (backtrack_active != 1) {
        // Not active yet: start if it is worth it, with the box at the target's current position.
        if (!entity_reach_check(ctx, target, eye)) {
            backtrack_render_box = 0;
            unlock();
            return;
        }
        backtrack_active = 1;
        clear_tracked_offset();
        Vec3 position = java_get_entity_pos_x(ctx->jvm, target);
        backtrack_previous_current_server_position = position;
        backtrack_previous_current_server_position_in_reach_distance_checks_a = position;
        backtrack_previous_current_server_position_in_reach_distance_checks = position;
        backtrack_yaws = yaw;
        backtrack_yaws_in_reach_distance_checks_a = yaw;
        backtrack_yaws_in_reach_distance_checks = yaw;
        backtrack_render_box = backtrack_render_remote_pos;
        unlock();
        return;
    }

    // Active: where is the target really?
    Vec3 real_offset = backtrack_stop_on_velocity_setting_field_0x1f == 1 ? backtrack_accumulated_offset : vec3_add(backtrack_accumulated_offset, shown_pos);
    double further = segment_distance(eye, shown_pos, real_offset);
    if (-0.05 > further || old_target_id != target_id) {
        // Releasing would bring the target closer (or a new target): stop and cool down.
        clear_tracked_offset();
        backtrack_render_box = 0;
        backtrack_active = 0;
        backtrack_cooldown_ticks = (int)backtrack_cooldown;
        unlock();
        return;
    }
    backtrack_record_server_position(real_offset, backtrack_stop_on_velocity_setting_field_0x27 ? backtrack_stop_on_velocity_setting_field_0x23 : yaw);
    unlock();
}
