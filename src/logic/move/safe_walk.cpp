// Safe Walk - don't fall off block edges (like sneaking, without sneaking).
//
// The edge check itself is done by the engine while safe_walk_safe_walk_active is 1.
// This callback decides whether it should be active, and can slow you down while it is.
//
// Callback (descriptor slot +0x48):
//   safe_walk_on_tick  every player tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" float safe_walk_motion_multiplier_value;              // "motion_multiplier": speed while active (1 = normal)
extern "C" unsigned char safe_walk_require_looking_down_value;   // "require_looking_down": only when pitch >= 65
extern "C" unsigned char safe_walk_require_not_forward_value;    // "require_not_forward": not while walking forwards
extern "C" unsigned char safe_walk_require_holding_blocks_value; // "require_holding_blocks"

// 1 = the engine stops the player at block edges.
extern "C" unsigned char safe_walk_safe_walk_active;

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct WalkState { char padding_00[8]; Ctx *ctx; };   // +0x08 game context

// ---- game / engine functions ----
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(float, java_get_entity_living_base_move_forward, (void *jvm, void *entity_living_base));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, block_entity_id, (void *jvm, void *item));   // 1 if the item is a block
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void, java_set_entity_motion_x, (void *jvm, void *entity, Vec3 value));

extern "C" void safe_walk_on_tick(WalkState *s) {
    Ctx *ctx = s->ctx;
    if (ctx->in_game != 1)
        return;

    // Turn off when a requirement is not met.
    if (safe_walk_require_looking_down_value == 1 && get_entity_rotation(ctx->jvm, ctx->player).pitch < 65.0f) {
        safe_walk_safe_walk_active = 0;
        return;
    }
    if (safe_walk_require_not_forward_value == 1 && java_get_entity_living_base_move_forward(ctx->jvm, ctx->player) > 0.0f) {
        safe_walk_safe_walk_active = 0;
        return;
    }
    if (safe_walk_require_holding_blocks_value == 1) {
        void *item = item_held_hotbar_slot(ctx->jvm, ctx->player);
        if (!block_entity_id(ctx->jvm, item)) {
            safe_walk_safe_walk_active = 0;
            return;
        }
    }
    safe_walk_safe_walk_active = 1;

    // Optionally slow down while on the ground.
    if (!java_get_entity_on_ground(ctx->jvm, ctx->player))
        return;
    if (safe_walk_motion_multiplier_value == 1.0f)
        return;
    Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
    double mult = safe_walk_motion_multiplier_value;
    motion.x *= mult;
    motion.z *= mult;
    java_set_entity_motion_x(ctx->jvm, ctx->player, motion);
}
