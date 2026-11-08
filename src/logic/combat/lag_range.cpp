// Lag Range - fake lag while walking up to your target, so you reach them before they see you coming.
//
// While a target is within 10-15 blocks and you are moving closer to it, your outgoing
// movement packets are held back for up to "delay" ms. The server (and the target) see you
// further away than you are, then you "teleport" in when the packets are released.
// Packets are released at once when you attack, stop sprinting ("flush_sprint_reset") or
// throw a splash potion ("flush_splash_potion").
//
// The symbol names "lag_range_target_position*" / "..._from_packets" are historical: they hold
// OUR position as last sent to the server (see the comments below).
//
// Callbacks:
//   lag_range_attack_hook                every attack: did the hit land this tick?
//   lag_range_render_box_interpolated    every frame: draw a box where the server thinks we are
//   lag_range_packet_hook_hold           every outgoing packet (network thread): hold or release
//   lag_range_tick_decide_whether        every tick: may we hold packets now?
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Range { float lo, hi; };

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
    int verdict;            // +0x24 set to 1 to hold the packet
};

struct PhaseHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10 1 = the attack happens
};

// ---- settings ----
extern "C" float lag_range_delay;                              // "delay": max hold (ms)
extern "C" unsigned char lag_range_require_weapon_value;       // "require_weapon"
extern "C" unsigned char lag_range_flush_splash_potion_value;  // "flush_splash_potion"
extern "C" unsigned char lag_range_flush_sprint_reset_value;   // "flush_sprint_reset"
extern "C" unsigned char lag_range_render_remote_pos;          // "render_remote_pos": draw the box (third person)
extern "C" float lag_range_line_width_setting;                 // "line_width"
extern "C" float lag_range_color_setting[3];                   // "color" (r, g, b)
extern "C" unsigned char lag_range_filled_setting;             // "filled"

// ---- module state (shared with the network thread, guarded by the spin lock) ----
extern "C" unsigned char lag_range_spin_lock;
extern "C" unsigned char lag_range_active;                       // holding is allowed this tick
extern "C" long long lag_range_max_hold_budget;                  // "delay" as a whole number
extern "C" int lag_range_last_packet_sequence;
extern "C" unsigned char lag_range_target_moved_closer;          // our last movement packet brought us closer to the target
extern "C" unsigned char lag_range_hold_active;                  // release on our next attack packet
extern "C" unsigned char lag_range_track_entity_action;          // release on our next "stop sprinting" packet
extern "C" unsigned char lag_range_hold_just_released;           // released, don't hold again until the next movement packet
extern "C" Vec3 lag_range_target_position_when_window_started;   // the target's position this tick
extern "C" Vec3 lag_range_latest_target_position_from_packets;   // OUR position in the last movement packet sent
extern "C" unsigned char lag_range_hit_time_unchanged;           // we landed a hit this tick
extern "C" unsigned char lag_range_sprinting_last_tick;
extern "C" int lag_range_pause_ticks;                            // ticks to pause after a splash potion
extern "C" char lag_range_random_walk_1[];                       // random walk state
// For the box: our server-side position last tick and this tick.
extern "C" unsigned char lag_range_have_target_position;
extern "C" Vec3 lag_range_target_position;
extern "C" Vec3 lag_range_target_position_second;

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlScalef)(float, float, float);
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);
FN(void, gl_array_xyz_float, ());          // set up the unit box vertex array
FN(void, gl_array_color1, ());             // draw it filled
FN(void, gl_array_color2, ());             // draw its outline
FN(void, disable_vertex_array_state, ());

// ---- game / engine functions ----
FN(void *, java_jni_is_instance_of_c03_packet_player, (void *ctx, void *event));        // movement packet, or null
FN(unsigned char, java_get_c03_packet_player_moving, (void *ctx, void *c03_packet_player));   // contains a position
FN(Vec3, java_get_c03_packet_player_a, (void *ctx, void *c03_packet_player));           // its position
FN(void *, java_jni_is_instance_of_c02_packet_use_entity, (void *ctx, void *event));    // attack packet, or null
FN(void *, java_jni_is_instance_of_c0_bpacket_entity_action, (void *ctx, void *event)); // sprint/sneak packet, or null
FN(int, java_c0_bpacket_entity_action, (void *ctx, void *object));                      // its action, 4 = stop sprinting
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned, java_get_item_stack_metadata, (void *jvm, void *stack));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));                               // next value of a random walk
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(int, java_get_game_settings_third_person_view, (void *jvm, void *game_settings));
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));   // server-side position
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack

enum { ITEM_POTION = 373, POTION_SPLASH_FLAG = 0x4000 };

static void lock() {
    while (__atomic_exchange_n(&lag_range_spin_lock, (unsigned char)1, __ATOMIC_SEQ_CST))
        __builtin_ia32_pause();
}
static void unlock() { lag_range_spin_lock = 0; }

extern "C" void lag_range_attack_hook(PhaseHook *hook) {
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;
    int now = java_get_entity_ticks_existed(ctx->jvm, ctx->player);
    lag_range_hit_time_unchanged = now == java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
}

// Every frame: a box (0.6 wide, 2 tall) where the server thinks we are.
extern "C" void lag_range_render_box_interpolated(RenderHook *hook) {
    if (lag_range_have_target_position != 1)
        return;
    Vec3 pos = vec3_sub(vec3_lerp(lag_range_target_position, lag_range_target_position_second, (double)hook->partial), hook->cam);
    gl_array_xyz_float();
    (*ptr_to_pGlLineWidth)(lag_range_line_width_setting);
    (*ptr_to_pGlPushMatrix)();
    (*ptr_to_pGlTranslatef)((float)pos.x, (float)(pos.y + 1.0), (float)pos.z);
    (*ptr_to_pGlScalef)(0.3f, 1.0f, 0.3f);
    if (lag_range_filled_setting == 1) {
        (*ptr_to_pGlColor4f)(lag_range_color_setting[0], lag_range_color_setting[1], lag_range_color_setting[2], 0.4f);
        gl_array_color1();
    }
    (*ptr_to_pGlColor3f)(lag_range_color_setting[0], lag_range_color_setting[1], lag_range_color_setting[2]);
    gl_array_color2();
    (*ptr_to_pGlPopMatrix)();
    disable_vertex_array_state();
}

// Every outgoing packet (network thread).
extern "C" void lag_range_packet_hook_hold(PktHook *hook) {
    lock();
    unsigned char active = lag_range_active;

    // A new packet: if it is a movement packet, are we moving closer to the target?
    if (hook->seq > lag_range_last_packet_sequence) {
        if (active) {
            void *movement = java_jni_is_instance_of_c03_packet_player(hook->ctx, hook->ev);
            if (movement && java_get_c03_packet_player_moving(hook->ctx, movement)) {
                Vec3 new_pos = java_get_c03_packet_player_a(hook->ctx, movement);
                double old_distance = vec3_distance_sq(lag_range_target_position_when_window_started, lag_range_latest_target_position_from_packets);
                lag_range_target_moved_closer = old_distance > vec3_distance_sq(lag_range_target_position_when_window_started, new_pos);
            }
        }
        lag_range_last_packet_sequence = hook->seq;
        active = lag_range_active;
    }

    if (active && lag_range_target_moved_closer && !lag_range_hold_active && !lag_range_hold_just_released &&
        !lag_range_track_entity_action && hook->budget < lag_range_max_hold_budget) {
        hook->verdict = 1;   // hold it
    } else if (hook->verdict == 0) {
        // The packet goes out now. Watch for the packets that release the hold.
        void *movement = java_jni_is_instance_of_c03_packet_player(hook->ctx, hook->ev);
        if (movement) {
            lag_range_hold_just_released = 0;
            if (java_get_c03_packet_player_moving(hook->ctx, movement))
                lag_range_latest_target_position_from_packets = java_get_c03_packet_player_a(hook->ctx, movement);
        } else if (java_jni_is_instance_of_c02_packet_use_entity(hook->ctx, hook->ev)) {
            // Attack: release.
            if (lag_range_hold_active) {
                lag_range_hold_active = 0;
                lag_range_hold_just_released = 1;
            }
        } else {
            // Stop sprinting: release.
            void *action = java_jni_is_instance_of_c0_bpacket_entity_action(hook->ctx, hook->ev);
            if (action && lag_range_track_entity_action && java_c0_bpacket_entity_action(hook->ctx, action) == 4) {
                lag_range_track_entity_action = 0;
                lag_range_hold_just_released = 1;
            }
        }
    }
    unlock();
}

// Should holding be off this tick? (updates the splash potion pause)
static bool paused_or_not_ready(Ctx *ctx) {
    if (ctx->in_game != 1)
        return true;
    if (lag_range_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return true;
    if (lag_range_flush_splash_potion_value == 1) {
        // Holding a splash potion: pause for 15 ticks.
        void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, java_get_inventory_player_current_item(ctx->jvm, inventory));
        if (stack) {
            void *held = item_held_hotbar_slot(ctx->jvm, ctx->player);
            if (held && java_call_static_item_get_id_from_item(ctx->jvm, held) == ITEM_POTION && (java_get_item_stack_metadata(ctx->jvm, stack) & POTION_SPLASH_FLAG)) {
                lag_range_pause_ticks = 15;
                return true;
            }
        }
        int pause = lag_range_pause_ticks;
        if (pause > 0) {
            lag_range_pause_ticks = pause - 1;
            return true;
        }
    }
    return false;
}

// Every tick: may we hold packets now?
extern "C" void lag_range_tick_decide_whether(Hook *hook) {
    unsigned char hit_this_tick = lag_range_hit_time_unchanged;
    lag_range_hit_time_unchanged = 0;
    Ctx *ctx = hook->ctx;

    bool on = false;
    if (!paused_or_not_ready(ctx)) {
        // Stopped sprinting since last tick: release on the "stop sprinting" packet.
        unsigned char sprinting = java_call_entity_is_sprinting(ctx->jvm, ctx->player);
        unsigned char stopped_sprinting = (lag_range_sprinting_last_tick & 1) ? lag_range_flush_sprint_reset_value : 0;
        if (sprinting)
            stopped_sprinting = 0;
        lag_range_sprinting_last_tick = sprinting;

        // (eye position, not used)
        Vec3 eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
        if (java_variant(ctx->jvm))
            eye = vec3_add_eye_height_sneak(eye);
        (void)eye;

        // Target within a random 10-15 blocks.
        CombatTarget *target = (CombatTarget *)ctx->target;
        if (target->ent) {
            double distance_sq = target->dist2;
            double max_range = walk_update(lag_range_random_walk_1, rot_make_floats(10.0f, 15.0f), 10.0);
            if (!(distance_sq > max_range * max_range)) {
                // Release on our next attack only when the target can take damage again.
                unsigned char target_can_be_hurt = java_get_entity_living_base_hurt_time(ctx->jvm, target->ent) < 5;
                unsigned char show_box = 0;
                if (lag_range_render_remote_pos == 1)
                    show_box = java_get_game_settings_third_person_view(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)) != 0;
                hit_this_tick &= target_can_be_hurt;
                Vec3 target_pos = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, target->ent);

                lock();
                lag_range_max_hold_budget = (long long)lag_range_delay;
                lag_range_hold_active = hit_this_tick;
                lag_range_track_entity_action = stopped_sprinting;
                lag_range_active = 1;
                lag_range_target_position_when_window_started = target_pos;
                // For the box: last tick's and this tick's server-side position.
                lag_range_target_position = lag_range_have_target_position == 1 ? lag_range_target_position_second : lag_range_latest_target_position_from_packets;
                lag_range_target_position_second = lag_range_latest_target_position_from_packets;
                lag_range_have_target_position = show_box ? lag_range_target_moved_closer : 0;
                unlock();
                on = true;
            }
        }
    }
    if (on)
        return;

    lock();
    lag_range_hold_just_released = 0;
    lag_range_hold_active = 0;
    lag_range_track_entity_action = 0;
    lag_range_active = 0;
    unlock();
    lag_range_have_target_position = 0;
}
