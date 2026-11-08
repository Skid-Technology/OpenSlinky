// Velocity - take less knockback.
//
// Four ways ("mode"):
//   0 "reduce":  when hit from the front, multiply the sideways knockback by "reduce_amount"
//                (after "reduce_delay" ticks)
//   1 "jump":    jump the moment you get knocked back (a "jump reset"), which cancels most of it
//   2 "hit":     when you hit the target back, keep only 60% of your own motion (like a sprint hit)
//   3 "delayed": ignore the knockback and keep the motion you had the tick before
// "chance", "require_*" and the target conditions decide when it works.
//
// Callbacks:
//   velocity_disable            module switched off
//   velocity_attack             before our attack: save our motion
//   velocity_tick               every tick: save our motion
//   velocity_mode_3_delayed     every tick (mode 3)
//   velocity_packet_hook_watch  every incoming packet: notice knockback / explosions / teleports
//   velocity_mode_2_reduce      after our attack (mode 2)
//   velocity_shared_conditions  the "require_*" and target conditions
//   velocity_tick_mode_0        every tick (modes 0 and 1)
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Range { float lo, hi; };

struct PhaseHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10 1 = the attack happens
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct PktHook {
    char padding_00[8];
    void *ctx;              // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    char padding_18[12];
    int verdict;            // +0x24 non-zero: another module is holding it
};

// ---- settings ----
extern "C" unsigned velocity_mode;                          // "mode"
extern "C" float velocity_chance_setting;                   // "chance" (mode 3), in percent
extern "C" unsigned velocity_keep_vertical_1_air_2;         // mode 3: keep the old vertical motion too: 1 = in the air, 2 = always
extern "C" unsigned char velocity_require_moving_setting;   // "require_moving": only while walking forwards
extern "C" unsigned char velocity_require_weapon_value;     // "require_weapon"
extern "C" unsigned char velocity_require_mouse_value;      // "require_mouse"
extern "C" unsigned char velocity_require_target_value;     // "require_target": a player within 6 blocks in front of us
extern "C" unsigned char velocity_require_sprinting_jump_value;   // mode 1: only while sprinting
extern "C" unsigned char velocity_every_hit_setting;        // mode 2: on every hit, not only the first of a combo
extern "C" float velocity_reduce_chance_value;              // mode 0 chance, in percent
extern "C" float velocity_reduce_delay_value;               // mode 0 delay, ticks
extern "C" float velocity_reduce_amount;                    // mode 0 multiplier
extern "C" float velocity_fov_value;                        // mode 0: only when hit from within this angle (radians) of the front
extern "C" float velocity_jump_chance_value;                // mode 1 chance, in percent

// ---- module state ----
extern "C" char velocity_module_descriptor[];
extern "C" unsigned char velocity_tick_motion_saved;
extern "C" Vec3 velocity_motion_at_last_tick;
extern "C" Vec3 velocity_motion_at_our_last_hit;
extern "C" unsigned char velocity_reduced_this_combo;
extern "C" int velocity_delay_ticks;
extern "C" unsigned char velocity_jump_key_pressed_by_us;
extern "C" char velocity_random_walk_1[];
// Shared with the network thread:
extern "C" int velocity_own_entity_id;
extern "C" unsigned char velocity_strong_knockback;          // we just got knocked back (or a strong explosion)
extern "C" unsigned char velocity_own_velocity_packet_seen;  // we were just teleported by the server

// ---- game / engine functions ----
FN(unsigned char, velocity_shared_conditions_entry, (Ctx *ctx));                             // = velocity_shared_conditions
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                       // a mouse button is held
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(Rot, rot_look_at, (Vec3 from, Vec3 to));
FN(float, wrap_angle, (float a));                                                           // to -180..180
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, get_held_item, (void *jvm, void *item));                                          // here: the jump key
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));           // key physically held down
FN(unsigned char, is_key_down, (void *jvm, void *k));
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(void *, java_potion_potion_types, (void *jvm, int id));
FN(void *, java_call_entity_living_base_get_active_potion_effect, (void *jvm, void *entity_living_base, void *effect));
FN(double, libc_hypot, (double x, double z));
FN(float, libc_sinf, (float v));
FN(float, libc_cosf, (float v));
FN(double, libc_acos, (double v));
FN(float, java_get_entity_living_base_move_forward, (void *jvm, void *entity_living_base));
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(Range, rot_make_floats, (float lo, float hi));
FN(unsigned char, walk_should_change, (void *state, Range r, double p));                   // random yes/no
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void, java_set_entity_motion_x, (void *jvm, void *entity, Vec3 value));
FN(void *, java_jni_is_instance_of_s12_packet_entity_velocity, (void *ctx, void *event));  // knockback packet, or null
FN(int, java_get_s12_packet_entity_velocity_entity_id, (void *ctx, void *s12_packet_entity_velocity));
FN(void *, java_jni_is_instance_of_s27_packet_explosion, (void *ctx, void *event));
FN(float, java_get_s27_packet_explosion_f, (void *ctx, void *s27_packet_explosion));        // push x
FN(float, java_get_s27_packet_explosion_h, (void *ctx, void *s27_packet_explosion));        // push z
FN(void *, java_jni_is_instance_of_s08_packet_player_pos_look, (void *ctx, void *event));  // server teleported us

enum VelocityMode { MODE_REDUCE = 0, MODE_JUMP = 1, MODE_HIT = 2, MODE_DELAYED = 3 };
enum { POTION_JUMP_BOOST = 8 };

// A chance in percent: always yes above 99, otherwise a random walk around it.
static bool roll(float percent) {
    if (percent > 99.0f)
        return true;
    double p = (float)((double)percent * 0.01);
    return walk_should_change(velocity_random_walk_1, rot_make_floats((float)(-0.05 + p), (float)(p + 0.05)), 60.0);
}

// Did the knockback flag get raised? (and clear it)
static bool take_knockback_flag() {
    unsigned char expected = 1;
    return __atomic_compare_exchange_n(&velocity_strong_knockback, &expected, 0, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

extern "C" void velocity_disable(EvHook *hook) {
    if (hook->ev == velocity_module_descriptor)
        velocity_tick_motion_saved = 0;
}

extern "C" void velocity_attack(PhaseHook *hook) {
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;
    if (ctx->player)
        velocity_motion_at_our_last_hit = java_get_entity_motion_x(ctx->jvm, ctx->player);
}

extern "C" void velocity_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->player) {
        velocity_motion_at_last_tick = java_get_entity_motion_x(ctx->jvm, ctx->player);
        velocity_tick_motion_saved = 1;
    }
}

// Mode 3: put back the motion of the tick before.
extern "C" void velocity_mode_3_delayed(Hook *hook) {
    if (velocity_mode != MODE_DELAYED)
        return;
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    // Not right after a teleport.
    if (__atomic_load_n(&velocity_own_velocity_packet_seen, __ATOMIC_SEQ_CST) & 1) {
        __atomic_exchange_n(&velocity_own_velocity_packet_seen, (unsigned char)0, __ATOMIC_SEQ_CST);
        return;
    }
    if (velocity_tick_motion_saved != 1 || !velocity_shared_conditions_entry(ctx))
        return;
    if (!roll(velocity_chance_setting))
        return;

    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    motion.x = velocity_motion_at_last_tick.x;
    unsigned keep_vertical = velocity_keep_vertical_1_air_2;
    if (keep_vertical == 2 || (keep_vertical == 1 && !java_get_entity_on_ground(ctx->jvm, ctx->player)))
        motion.y = velocity_motion_at_last_tick.y;
    motion.z = velocity_motion_at_last_tick.z;
    java_set_entity_motion_x(ctx->jvm, ctx->player, motion);
    velocity_tick_motion_saved = 0;
}

// Every incoming packet (network thread).
extern "C" void velocity_packet_hook_watch(PktHook *hook) {
    if (hook->verdict != 0)
        return;

    // Knockback for us.
    void *knockback = java_jni_is_instance_of_s12_packet_entity_velocity(hook->ctx, hook->ev);
    if (knockback) {
        if (java_get_s12_packet_entity_velocity_entity_id(hook->ctx, knockback) == velocity_own_entity_id)
            __atomic_exchange_n(&velocity_strong_knockback, (unsigned char)1, __ATOMIC_SEQ_CST);
        return;
    }
    // An explosion pushing us noticeably.
    void *explosion = java_jni_is_instance_of_s27_packet_explosion(hook->ctx, hook->ev);
    if (explosion) {
        float push_x = java_get_s27_packet_explosion_f(hook->ctx, explosion);
        float push_z = java_get_s27_packet_explosion_h(hook->ctx, explosion);
        unsigned char strong = (double)(push_x * push_x + push_z * push_z) > 0.05;
        __atomic_exchange_n(&velocity_strong_knockback, strong, __ATOMIC_SEQ_CST);
        return;
    }
    if (java_jni_is_instance_of_s08_packet_player_pos_look(hook->ctx, hook->ev))
        __atomic_exchange_n(&velocity_own_velocity_packet_seen, (unsigned char)1, __ATOMIC_SEQ_CST);
}

// Mode 2: when our hit lands on a target, slow our own motion to 60% (as a sprint hit would).
extern "C" void velocity_mode_2_reduce(PhaseHook *hook) {
    if (velocity_mode != MODE_HIT)
        return;
    if (velocity_reduced_this_combo == 1 && !velocity_every_hit_setting)
        return;
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;
    if (!ctx->player || ((CombatTarget *)ctx->target)->valid != 1)
        return;
    if (velocity_require_moving_setting == 1 && 0.0f >= java_get_entity_living_base_move_forward(ctx->jvm, ctx->player))
        return;
    if (velocity_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return;
    if (java_get_entity_ticks_existed(ctx->jvm, ctx->player) != java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player))
        return;   // the hit did not land
    velocity_motion_at_our_last_hit.x *= 0.6;
    velocity_motion_at_our_last_hit.z *= 0.6;
    java_set_entity_motion_x(ctx->jvm, ctx->player, velocity_motion_at_our_last_hit);
    velocity_reduced_this_combo = 1;
}

// The "require_*" conditions, and "require_target": another player within 6 blocks
// and within 90 degrees of where we look.
extern "C" unsigned char velocity_shared_conditions(Ctx *ctx) {
    if (velocity_require_moving_setting == 1 && 0.0f >= java_get_entity_living_base_move_forward(ctx->jvm, ctx->player))
        return 0;
    if (velocity_require_mouse_value == 1 && !vertex_pointer_current(ctx))
        return 0;
    if (velocity_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return 0;
    if (velocity_require_target_value != 1)
        return 1;

    Vec3 my_pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Rot rotation = get_entity_rotation(ctx->jvm, ctx->player);
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        if ((*env)->IsSameObject(env, ctx->player, other))
            continue;   // ourselves
        Vec3 other_pos = java_get_entity_pos_x(ctx->jvm, other);
        if (vec3_distance_sq(my_pos, other_pos) > 36.0)
            continue;
        float angle = __builtin_fabsf(wrap_angle(rot_look_at(my_pos, other_pos).yaw - rotation.yaw));
        if (angle > 90.0f)
            continue;
        return 1;
    }
    return 0;
}

// Mode 1: jump on knockback.
static void jump_reset(Ctx *ctx) {
    // We pressed jump last tick: give the key back.
    if (velocity_jump_key_pressed_by_us == 1) {
        void *jump_key = get_held_item(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
        java_set_key_binding_pressed(ctx->jvm, jump_key, java_get_key_binding_key_code(ctx->jvm, jump_key));
        velocity_jump_key_pressed_by_us = 0;
        return;
    }
    // Only as a reaction to knockback (the packet hook raises the flag).
    if (!take_knockback_flag())
        return;
    if (!velocity_shared_conditions(ctx) || !roll(velocity_jump_chance_value))
        return;
    if (velocity_require_sprinting_jump_value == 1 && !java_call_entity_is_sprinting(ctx->jvm, ctx->player))
        return;
    if (!java_get_entity_on_ground(ctx->jvm, ctx->player))
        return;
    if (0.0 < java_get_entity_motion_x(ctx->jvm, ctx->player).y)
        return;
    if (java_call_entity_living_base_get_active_potion_effect(ctx->jvm, ctx->player, java_potion_potion_types(ctx->jvm, POTION_JUMP_BOOST)))
        return;
    void *jump_key = get_held_item(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
    if (is_key_down(ctx->jvm, jump_key))
        return;
    java_set_key_binding_pressed(ctx->jvm, jump_key, 1);
    velocity_jump_key_pressed_by_us = 1;
}

// Mode 0: after knockback from the front, reduce the sideways motion after "reduce_delay" ticks.
static void reduce_after_delay(Ctx *ctx) {
    int ticks = velocity_delay_ticks;
    bool apply_now = false;
    if (ticks <= 0) {
        // Not waiting: start on new knockback.
        if (!take_knockback_flag())
            return;
        if (!velocity_shared_conditions(ctx) || !roll(velocity_reduce_chance_value))
            return;
        Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
        double speed = libc_hypot(motion.x, motion.z);
        if (speed == 0.0)
            return;
        // Angle between where we look and where we are pushed: must be within "fov".
        float yaw = get_entity_rotation(ctx->jvm, ctx->player).yaw;
        float facing = (float)((double)yaw * -3.141592653589793 / 180.0 + -3.141592653589793);
        float sin_facing = libc_sinf(facing);
        float cos_facing = libc_cosf(facing);
        double angle = libc_acos(((double)sin_facing * motion.x + (double)cos_facing * motion.z) / speed);
        if (!((double)velocity_fov_value >= angle))
            return;
        ticks = (int)velocity_reduce_delay_value;
        velocity_delay_ticks = ticks;
        apply_now = ticks <= 0;
    }
    if (!apply_now) {
        velocity_delay_ticks = ticks - 1;
        if (ticks - 1 != 0)
            return;
    }
    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    double amount = velocity_reduce_amount;
    motion.x *= amount;
    motion.z *= amount;
    java_set_entity_motion_x(ctx->jvm, ctx->player, motion);
}

// Every tick (modes 0 and 1).
extern "C" void velocity_tick_mode_0(Hook *hook) {
    velocity_reduced_this_combo = 0;
    if (velocity_mode == MODE_HIT)
        return;
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    __atomic_exchange_n(&velocity_own_entity_id, java_get_entity_entity_id(ctx->jvm, ctx->player), __ATOMIC_SEQ_CST);
    unsigned mode = velocity_mode;
    if (mode == MODE_JUMP)
        jump_reset(ctx);
    else if (mode == MODE_REDUCE)
        reduce_after_delay(ctx);
}
