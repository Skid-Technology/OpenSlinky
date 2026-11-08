// Strafe - change direction instantly, even in the air.
//
// Minecraft keeps your momentum: in the air you can barely steer. This module turns your
// horizontal motion towards the direction your keys point, keeping the same speed.
// "strength_*" sets how far it turns (0 = not at all, 1 = fully) on the ground, in the air,
// and on the tick you jump.
//
// Callbacks (descriptor slots +0x198 / +0x1a8):
//   strafe_on_movement_input  every movement update: remember whether we were sprinting
//   strafe_on_move_flying     every movement update: steer the motion
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };

struct MoveHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    float strafe;           // +0x10 sideways input (+ = left, - = right)
    float forward;          // +0x14 forwards input (+ = forward, - = back)
    float speed;            // +0x18
    unsigned char sprint;   // +0x1c 1 on the tick the player jumps
};

// ---- settings ----
extern "C" float strafe_max_hurt_time_value;       // "max_hurt_time": don't steer while taking knockback
extern "C" float strafe_strength_air_value;        // "strength_air"
extern "C" float strafe_strength_jump_value;       // "strength_jump"
extern "C" float strafe_strength_ground_value;     // "strength_ground"
extern "C" unsigned char strafe_require_weapon_value;   // "require_weapon"

// Sprinting state at the last movement input (despite the name).
extern "C" unsigned char strafe_was_on_ground_last_tick;

// ---- game / engine functions ----
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void, java_set_entity_motion_x, (void *jvm, void *entity, Vec3 value));
FN(double, libc_hypot, (double x, double z));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(float, libc_sinf, (float v));
FN(float, libc_cosf, (float v));
FN(double, lerp_double, (double a, double b, double c));   // a + (b - a) * c

extern "C" void strafe_on_movement_input(Hook *hook) {
    Ctx *ctx = hook->ctx;
    strafe_was_on_ground_last_tick = java_call_entity_is_sprinting(ctx->jvm, ctx->player);
}

extern "C" void strafe_on_move_flying(MoveHook *hook) {
    if (hook->forward == 0.0f && hook->strafe == 0.0f)
        return;
    Ctx *ctx = hook->ctx;
    if (strafe_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return;
    if ((float)java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) > strafe_max_hurt_time_value)
        return;

    // Pick the strength for this tick. Jumping without sprint gets the sprint-jump boost added.
    float strength;
    bool add_jump_boost = false;
    if (java_get_entity_on_ground(ctx->jvm, ctx->player)) {
        if (hook->sprint == 1) {
            add_jump_boost = strafe_was_on_ground_last_tick ^ 1;
            strength = strafe_strength_jump_value;
        } else {
            strength = strafe_strength_ground_value;
        }
    } else {
        strength = strafe_strength_air_value;
    }
    if (strength == 0.0f)
        return;

    // Keep the current speed.
    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    double speed = libc_hypot(motion.x, motion.z);
    if (add_jump_boost)
        speed += 0.2;

    // Direction the keys point, in degrees: the camera yaw, turned for the keys held.
    float yaw = get_entity_rotation(ctx->jvm, ctx->player).yaw;
    double side = 0.0;   // -1 = right, +1 = left
    if (0.0f > hook->strafe)
        side = -1.0;
    if (hook->strafe > 0.0f)
        side = 1.0;
    double direction = yaw;
    if (hook->forward > 0.0f) {
        side *= -45.0;        // diagonal forward
    } else if (0.0f > hook->forward) {
        direction += 180.0;   // walking backwards
        side *= 45.0;         // diagonal backward
    } else {
        side *= -90.0;        // straight sideways
    }
    direction += side;
    float radians = (float)(direction * 3.141592653589793 / 180.0);

    // Turn the motion towards that direction.
    motion.x = lerp_double(motion.x, (double)-libc_sinf(radians) * speed, strength);
    motion.z = lerp_double(motion.z, (double)libc_cosf(radians) * speed, strength);
    java_set_entity_motion_x(ctx->jvm, ctx->player, motion);
}
