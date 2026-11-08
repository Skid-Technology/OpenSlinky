// Reach - hit players from further away.
//
// The engine asks every tick how far attacks may reach, and gets "distance" back.
// Each hit you land starts a 9-tick cooldown; the longer reach is only given once the
// cooldown is below "activate_time" (so right after a hit it is off for a few ticks).
// Optionally only while sprinting.
//
// Callback (descriptor slot +0xc8):
//   reach_on_reach_query  every tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" float reach_distance;                 // "distance": reach in blocks
extern "C" float reach_activate_time;            // "activate_time": ticks after a hit with the longer reach
extern "C" unsigned char reach_require_sprint;   // "require_sprint"

// ---- module state ----
extern "C" int reach_last_value_seen_from_java;  // tick of our last attack, as seen last time
extern "C" int reach_cooldown_ticks;             // counts down from 9 after each attack

// ---- game / engine functions ----
FN(bool, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack

// The engine's question, filled in by the callback.
struct ReachState {
    char padding_00[8];
    Ctx *ctx;           // +0x08 game context
    bool active;        // +0x10 set to true to change the reach
    char padding_11[7];
    double reach;       // +0x18 the reach to use
};

extern "C" void reach_on_reach_query(ReachState *s) {
    int cooldown = reach_cooldown_ticks;
    if (cooldown > 0) {
        cooldown--;
        reach_cooldown_ticks = cooldown;
    }

    Ctx *ctx = s->ctx;
    if (reach_require_sprint == 1) {
        if (!java_call_entity_is_sprinting(ctx->jvm, ctx->player))
            return;
        cooldown = reach_cooldown_ticks;
    }

    // Cooldown over: a new attack starts it again.
    if (cooldown == 0) {
        int last_attack = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
        if (last_attack == reach_last_value_seen_from_java) {
            cooldown = reach_cooldown_ticks;
        } else {
            reach_last_value_seen_from_java = last_attack;
            reach_cooldown_ticks = 9;
            cooldown = 9;
        }
    }

    // The cooldown counts down from 9: the longer reach is on once it is below "activate_time".
    if (reach_activate_time > (float)cooldown) {
        s->active = true;
        s->reach = (double)reach_distance;
    }
}
