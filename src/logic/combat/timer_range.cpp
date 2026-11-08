// Timer Range - freeze the game for a moment, then catch up in a burst that carries you into hit range.
//
// When a target is a few ticks of walking away, the module works out how many ticks of
// walking forwards would bring it into reach. It then freezes the game timer for that many
// ticks' worth of time and plays them all at once, so you arrive in range (and attack)
// before the target can react.
//   mode 0: freeze first, then play the ticks quickly
//   mode 1: play the ticks at once, then freeze (holding packets) to even out the time
//
// Callbacks:
//   timer_range_on_packet_sent   every outgoing packet: hold it during a mode 1 freeze
//   timer_range_on_timer_phase   module switched off: restore the timer
//   timer_range_hypot            helper: start the freeze
//   timer_range_tick             every tick: during a burst attack; otherwise check if a burst is possible
//   timer_range_render_tick      every frame: end the freeze; simulate ticks ahead to plan a burst
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Box { Vec3 lo, hi; };

struct FrameEvt {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    float partial;       // +0x10 how far we are between two ticks (0..1)
    int padding_14;
    double dt;           // +0x18 seconds since the last frame
};

struct PktHook { char padding_00[0x24]; int verdict; };   // +0x24 set to 1 to hold the packet

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" int timer_range_mode;                            // see above
extern "C" unsigned char timer_range_block_while_charging;  // block with the sword while frozen (mode 0)
extern "C" unsigned char timer_range_require_mouse_pressed;
extern "C" unsigned char timer_range_require_weapon;
extern "C" unsigned char timer_range_require_not_using_item;
extern "C" float timer_range_max_ticks, timer_range_max_ticks_second;   // max ticks to skip (mode 0 / mode 1)
extern "C" float timer_range_min_range;                     // don't burst when this close already
extern "C" float timer_range_cooldown_after_burst;          // seconds between bursts

// ---- module state ----
extern "C" char timer_range_module_descriptor[];
extern "C" int timer_range_delay_ticks;                     // ticks left in the burst
extern "C" unsigned char timer_range_timer_modified;        // the timer is frozen
extern "C" unsigned char timer_range_holding_packets;       // read by the network thread
extern "C" unsigned char timer_range_burst_would_reach_target;   // a burst is possible this tick
extern "C" float timer_range_last_forward_input, timer_range_last_strafe_input;
extern "C" double timer_range_cooldown;                     // seconds before the next burst
extern "C" double timer_range_freeze_time_left;             // seconds

// ---- game / engine functions ----
FN(void *, get_timer, (void *jvm, void *timer));                                          // the game's tick timer
FN(void, java_set_timer_timer_speed, (void *jvm, void *timer, float speed));
FN(void, java_set_timer_elapsed_partial_ticks, (void *jvm, void *timer, float ticks));   // ticks to run on the next frame
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, item_sword, (void *jvm, void *item));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));         // = one click
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *hit));           // 2 = an entity
FN(void, java_set_minecraft_left_click_counter, (void *jvm, void *minecraft, int n));
FN(void *, java_get_game_settings_attack_key, (void *jvm, void *game_settings));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                    // a mouse button is held
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void *, get_held_item, (void *jvm, void *game_settings));                              // here: the jump key
FN(unsigned char, is_key_down, (void *jvm, void *k));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height, (Vec3 v));
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));   // target's server position
FN(unsigned char, set_vertex_pointer, (void *jvm, void *world, Vec3 from, Vec3 to));     // clear line of sight between two points
FN(void, timer_range_read_movement_axes, (void *jvm, void *game_settings));               // fills timer_range_last_*_input
FN(int, java_get_entity_other_player_mp_other_player_mppos_rotation_increments, (void *jvm, void *entity_other_player_mp));   // interpolation steps left
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(float, libc_cosf, (float x));
FN(float, libc_sinf, (float x));
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Vec3, vec3_scale, (Vec3 v, double k));
FN(Vec3, vec3_reciprocal, (Vec3 v));
FN(unsigned char, aabb_ray_slab_test, (Box b, Vec3 origin, Vec3 dir));                    // does the ray hit the box?
FN(Vec3, aabb_clamp_point, (Box b, Vec3 p));                                               // closest point of the box
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(float, java_get_entity_living_base_land_movement_factor, (void *jvm, void *entity_living_base));
FN(Vec3, horizontal_motion, (Rot rotation, float fwd, float strafe, float speed));        // motion one tick of input adds
FN(double, libc_hypot, (double x, double y));

static const double DEGREES_TO_RADIANS = 0.017453292519943295;

extern "C" void timer_range_hypot(Ctx *ctx);

extern "C" void timer_range_on_packet_sent(PktHook *hook) {
    if (__atomic_load_n(&timer_range_holding_packets, __ATOMIC_SEQ_CST) & 1)
        hook->verdict = 1;
}

// On disable: normal timer speed, no burst.
extern "C" void timer_range_on_timer_phase(EvHook *hook) {
    if (hook->ev != timer_range_module_descriptor || hook->on)
        return;
    Ctx *ctx = hook->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);
    java_set_timer_timer_speed(ctx->jvm, timer, 1.0f);
    timer_range_timer_modified = 0;
    timer_range_delay_ticks = 0;
    __atomic_exchange_n(&timer_range_holding_packets, (unsigned char)0, __ATOMIC_SEQ_CST);
}

// Start the freeze (the timer stops; mode 1 also holds packets; mode 0 can block meanwhile).
extern "C" void timer_range_hypot(Ctx *c) {
    void *timer = get_timer(c->jvm, c->minecraft);
    java_set_timer_timer_speed(c->jvm, timer, 0.0f);
    timer_range_timer_modified = 1;
    if (timer_range_mode == 1) {
        __atomic_exchange_n(&timer_range_holding_packets, (unsigned char)1, __ATOMIC_SEQ_CST);
    } else if (timer_range_block_while_charging == 1 && item_sword(c->jvm, item_held_hotbar_slot(c->jvm, c->player))) {
        void *use_key = java_get_game_settings_use_item_key(c->jvm, get_game_settings(c->jvm, c->minecraft));
        java_set_key_binding_pressed(c->jvm, use_key, 1);
        java_increment_key_binding_press_time(c->jvm, use_key);
    }
}

extern "C" void timer_range_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    CombatTarget *target = (CombatTarget *)ctx->target;

    if (timer_range_delay_ticks > 0) {
        // In a burst: count it down (mode 0 freezes at the end), and attack when on the target.
        void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
        int ticks_left = --timer_range_delay_ticks;
        if (ticks_left == 0 && timer_range_mode == 0)
            timer_range_hypot(ctx);
        if (ctx->in_game != 1 || !target->ent)
            return;
        void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 2)
            return;
        java_set_minecraft_left_click_counter(ctx->jvm, ctx->minecraft, 0);
        java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_attack_key(ctx->jvm, settings));
        return;
    }

    // Is a burst possible? Target further than sqrt(3) blocks, we stand on the ground walking
    // forwards (not jumping), the target can be hurt, and we can see it.
    timer_range_burst_would_reach_target = 0;
    if (ctx->in_game != 1)
        return;
    if (!target->ent || 3.0 > target->dist2)
        return;
    if (!java_get_entity_on_ground(ctx->jvm, ctx->player) || java_get_entity_living_base_hurt_time(ctx->jvm, target->ent) > 1)
        return;
    if (timer_range_require_mouse_pressed == 1 && !vertex_pointer_current(ctx))
        return;
    if (timer_range_require_weapon == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return;
    if (timer_range_require_not_using_item == 1 && java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
        return;
    if (java_get_entity_motion_x(ctx->jvm, ctx->player).y > 0.0)
        return;
    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    if (is_key_down(ctx->jvm, get_held_item(ctx->jvm, settings)))
        return;   // jumping

    Vec3 my_eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (!java_variant(ctx->jvm))
        my_eye = vec3_add_eye_height(my_eye);
    Vec3 target_eye = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, target->ent);
    if (!java_variant(ctx->jvm))
        target_eye = vec3_add_eye_height(target_eye);
    if (!set_vertex_pointer(ctx->jvm, ctx->world, my_eye, target_eye))
        return;
    timer_range_read_movement_axes(ctx->jvm, settings);
    timer_range_burst_would_reach_target = timer_range_last_forward_input > 0.0f;
}

// End of the freeze: normal speed again.
static void end_freeze(Ctx *ctx, void *timer) {
    java_set_timer_timer_speed(ctx->jvm, get_timer(ctx->jvm, ctx->minecraft), 1.0f);
    timer_range_timer_modified = 0;
    __atomic_exchange_n(&timer_range_holding_packets, (unsigned char)0, __ATOMIC_SEQ_CST);
    int mode = timer_range_mode;
    if (mode != 1 && timer_range_block_while_charging == 1) {
        if (item_sword(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player)))
            java_set_key_binding_pressed(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)), 0);
        mode = timer_range_mode;
    }
    // Mode 1: now play the burst's ticks at once.
    if (mode == 1)
        java_set_timer_elapsed_partial_ticks(ctx->jvm, timer, (float)timer_range_delay_ticks);
}

// Every frame.
extern "C" void timer_range_render_tick(FrameEvt *h) {
    Ctx *ctx = h->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);
    if (timer_range_timer_modified == 1) {
        timer_range_freeze_time_left -= h->dt;
        if (timer_range_freeze_time_left > 0.0) {
            java_set_timer_timer_speed(ctx->jvm, timer, 0.0f);
            return;
        }
        end_freeze(ctx, timer);
    }
    if (timer_range_cooldown > 0.0) {
        timer_range_cooldown -= h->dt;
        return;
    }
    if (timer_range_burst_would_reach_target != 1 || ctx->in_game != 1)
        return;
    CombatTarget *target = (CombatTarget *)ctx->target;
    if (!target->ent)
        return;

    // Simulate the next ticks: us walking forwards, the target moving to its server position.
    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    Vec3 my_eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (!java_variant(ctx->jvm))
        my_eye = vec3_add_eye_height(my_eye);
    Vec3 target_pos = java_get_entity_pos_x(ctx->jvm, target->ent);
    Vec3 target_server_pos = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, target->ent);
    int interpolation_steps = java_get_entity_other_player_mp_other_player_mppos_rotation_increments(ctx->jvm, target->ent);

    // Our look direction (unit vector, as Minecraft computes it).
    Rot rotation = get_entity_rotation(ctx->jvm, ctx->player);
    float yaw_radians = (float)((double)-3.14159265f - (double)rotation.yaw * DEGREES_TO_RADIANS);
    float cos_yaw = libc_cosf(yaw_radians), sin_yaw = libc_sinf(yaw_radians);
    float pitch_radians = (float)((double)-rotation.pitch * DEGREES_TO_RADIANS);
    float minus_cos_pitch = -libc_cosf(pitch_radians), sin_pitch = libc_sinf(pitch_radians);
    Vec3 look = {(double)(sin_yaw * minus_cos_pitch), (double)sin_pitch, (double)(cos_yaw * minus_cos_pitch)};

    int max_ticks = (int)(timer_range_mode == 0 ? timer_range_max_ticks : timer_range_max_ticks_second);
    for (int ticks = 1; ticks <= max_ticks; ticks++) {
        // Would we hit the target's hitbox from here (looking 3 blocks ahead)?
        Box hitbox;
        hitbox.lo = vec3_sub(target_pos, vec3_make(0.4, 0.1, 0.4));
        hitbox.hi = vec3_add(target_pos, vec3_make(0.4, 1.9, 0.4));
        Vec3 inverse_ray = vec3_reciprocal(vec3_scale(look, 3.0));
        if (aabb_ray_slab_test(hitbox, my_eye, inverse_ray)) {
            double distance_sq = vec3_distance_sq(my_eye, aabb_clamp_point(hitbox, my_eye));
            if (distance_sq > 0.0) {
                if ((double)(timer_range_min_range * timer_range_min_range) > distance_sq)
                    return;   // already close enough
                // Plan the burst of `ticks` ticks.
                if (timer_range_mode == 1)
                    timer_range_hypot(ctx);
                else
                    java_set_timer_elapsed_partial_ticks(ctx->jvm, timer, (float)ticks);
                timer_range_delay_ticks = ticks;
                timer_range_freeze_time_left = (double)((float)ticks - h->partial) * 0.05;   // 0.05 s per tick
                timer_range_cooldown = timer_range_cooldown_after_burst;
                return;
            }
        }

        // One tick of our movement (ground friction 0.546)...
        Vec3 push = horizontal_motion(rotation, timer_range_last_forward_input, timer_range_last_strafe_input, java_get_entity_living_base_land_movement_factor(ctx->jvm, ctx->player));
        motion = vec3_add(motion, push);
        my_eye = vec3_add(my_eye, motion);
        motion = vec3_scale(motion, 0.546);
        if (timer_range_mode == 1) {
            // (mode 1: assume the target walks towards us at our speed)
            double speed = libc_hypot(motion.x, motion.z);
            Vec3 towards_us = vec3_sub(my_eye, target_server_pos);
            Vec3 step = vec3_scale(towards_us, speed / libc_hypot(towards_us.x, towards_us.z));
            step.y = 0.0;
            target_server_pos = vec3_add(target_server_pos, step);
        }
        // ...and the target sliding towards its server position.
        if (interpolation_steps > 0) {
            Vec3 remaining = vec3_sub(target_server_pos, target_pos);
            target_pos = vec3_add(target_pos, vec3_scale(remaining, 1.0 / (double)interpolation_steps));
            interpolation_steps -= 1;
        }
    }
}
