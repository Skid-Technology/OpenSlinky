// Keep Sprint - don't lose speed when you hit someone while sprinting.
//
// Hitting a player while sprinting normally slows you down and stops your sprint.
// This module saves your motion just before the hit and puts (part of) it back after.
//
// Callbacks (descriptor slots +0x128 / +0x138 / +0x158):
//   keep_sprint_on_attack         just before an attack: roll "chance", save the motion
//   keep_sprint_on_attack_post    just after the attack: restore the saved motion
//   keep_sprint_on_player_update  next player tick: turn sprint back on
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };

struct PhaseHook {
    char padding_00[8];
    Ctx *ctx;              // +0x08 game context
    unsigned char phase;   // +0x10 1 = the hit actually happens
};

// ---- settings ----
extern "C" float keep_sprint_chance_value;                  // "chance": how often it works
extern "C" float keep_sprint_motion_mult;                   // "motion_mult": how much of the saved speed to give back
extern "C" unsigned char keep_sprint_require_on_ground_value;   // "require_on_ground"
extern "C" unsigned char keep_sprint_allow_reduce_value;        // "allow_reduce": let the hit slow you when you have a target

// ---- module state ----
extern "C" Vec3 keep_sprint_saved_motion;                   // motion saved before the hit
extern "C" unsigned char keep_sprint_roll_succeeded;        // the "chance" roll succeeded for this hit
extern "C" unsigned char keep_sprint_motion_pending_re_apply;   // motion restored, sprint needs turning back on

// ---- game / engine functions ----
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(unsigned char, random_chance, (float chance));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void, java_set_entity_motion_x, (void *jvm, void *entity, Vec3 value));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(void, java_call_entity_set_sprinting, (void *jvm, void *entity, bool on));

// Before the hit: save the motion if we are sprinting and the roll succeeds.
extern "C" void keep_sprint_on_attack(PhaseHook *hook) {
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;
    unsigned char success = 0;
    if (ctx->in_game == 1 && java_call_entity_is_sprinting(ctx->jvm, ctx->player) && random_chance(keep_sprint_chance_value)) {
        keep_sprint_saved_motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
        success = 1;
    }
    keep_sprint_roll_succeeded = success;
}

// After the hit: put the saved motion back.
extern "C" void keep_sprint_on_attack_post(PhaseHook *hook) {
    if (keep_sprint_roll_succeeded != 1 || hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;
    if (!ctx->player)
        return;
    if (keep_sprint_require_on_ground_value == 1 && !java_get_entity_on_ground(ctx->jvm, ctx->player))
        return;
    // Only if the attack really landed this tick.
    if (java_get_entity_ticks_existed(ctx->jvm, ctx->player) != java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player))
        return;
    if (keep_sprint_allow_reduce_value == 1 && ((CombatTarget *)ctx->target)->valid)
        return;

    double mult = keep_sprint_motion_mult;
    keep_sprint_saved_motion.x *= mult;
    keep_sprint_saved_motion.z *= mult;
    java_set_entity_motion_x(ctx->jvm, ctx->player, keep_sprint_saved_motion);
    keep_sprint_motion_pending_re_apply = 1;
}

// Next player tick: the hit turned sprint off, turn it back on.
extern "C" void keep_sprint_on_player_update(Hook *hook) {
    if (keep_sprint_motion_pending_re_apply != 1)
        return;
    keep_sprint_motion_pending_re_apply = 0;
    Ctx *ctx = hook->ctx;
    if (ctx->player)
        java_call_entity_set_sprinting(ctx->jvm, ctx->player, true);
}
