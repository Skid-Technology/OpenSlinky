// Fast Accel - reach full walking speed faster on the ground.
//
// Normally Minecraft needs several ticks to speed up from standing to full speed.
// This module works out the top speed the current keys would eventually reach, and
// moves the player's horizontal speed part of the way there ("acceleration" = how far).
//
// Callback (descriptor slot +0x1a8):
//   fast_accel_on_move_flying  every movement update, before the game applies the input
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };

// What the engine hands to the callback.
struct MoveHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    float strafe;           // +0x10 sideways input
    float forward;          // +0x14 forwards input
    float speed;            // +0x18 movement speed for this tick
    unsigned char sprint;   // +0x1c 1 when the game treats this tick as sprinting
};

// ---- settings ----
extern "C" float fast_accel_acceleration_value;                  // "acceleration": 0 = no change, 1 = jump straight to top speed
extern "C" unsigned char fast_accel_require_weapon_value;        // "require_weapon": only while holding a weapon
extern "C" unsigned char fast_accel_require_not_damaged_value;   // "require_not_damaged": not while taking knockback

// ---- game / engine functions ----
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));                // the player's motion (x, y, z)
FN(void, java_set_entity_motion_x, (void *jvm, void *entity, Vec3 value));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(double, libc_hypot, (double x, double z));                                // sqrt(x*x + z*z)
FN(float, libc_hypotf_alt, (float x, float y));                              // same, in float
FN(double, lerp_double, (double a, double b, double c));                     // a + (b - a) * c

// Minecraft's slow-down per tick on the ground (block slipperiness 0.6 * air drag 0.91).
static const double GROUND_FRICTION = 0.546;

extern "C" void fast_accel_on_move_flying(MoveHook *hook) {
    // Not pressing any movement key.
    if (hook->forward == 0.0f && hook->strafe == 0.0f)
        return;

    Ctx *ctx = hook->ctx;
    if (fast_accel_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return;
    if (fast_accel_require_not_damaged_value == 1 && java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) > 0)
        return;

    // Current horizontal speed.
    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    double current_speed = libc_hypot(motion.x, motion.z);
    if (current_speed == 0.0)
        return;

    if (!java_get_entity_on_ground(ctx->jvm, ctx->player))
        return;

    // How much speed the keys add each tick (same maths as Minecraft's moveFlying):
    // the input direction, normalised so diagonals are not faster, times the speed.
    float input_length = libc_hypotf_alt(hook->strafe, hook->forward);
    float scale = hook->speed / (input_length > 1.0f ? input_length : 1.0f);
    float push_strafe = hook->strafe * scale;
    float push_forward = scale * hook->forward;
    double push_per_tick = libc_hypotf_alt(push_strafe, push_forward);

    // Speed after 20 ticks of: speed = speed * friction + push.
    double top_speed = 0.0;
    top_speed += push_per_tick;
    for (int tick = 0; tick < 19; tick++) {
        top_speed *= GROUND_FRICTION;
        top_speed += push_per_tick;
    }
    if (hook->sprint == 1 && java_call_entity_is_sprinting(ctx->jvm, ctx->player))
        top_speed += 0.2;   // sprint-jump bonus

    // Already at (or above) top speed: leave it alone.
    if (current_speed > top_speed)
        return;

    // Keep the direction, change only the speed.
    double new_speed = lerp_double(current_speed, top_speed, fast_accel_acceleration_value);
    motion.x = motion.x / current_speed * new_speed;
    motion.z = motion.z / current_speed * new_speed;
    java_set_entity_motion_x(ctx->jvm, ctx->player, motion);
}
