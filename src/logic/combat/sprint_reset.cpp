// Sprint Reset - stop sprinting for a moment after each hit, so the next hit deals full knockback.
//
// Knockback is extra strong on the first hit after you start sprinting. After you land a hit,
// this module waits "delay" ticks, then lets go of a key for "duration" ticks (which stops the
// sprint), then gives the key back. Which key depends on "mode":
//   0 = release forward, 1 = press sneak, 2 = release the sprint key (and stop sprinting)
//
// Callback (descriptor slot +0x158):
//   sprint_reset_on_player_update  every player tick
#include "slinky/engine.h"

struct Range { float lo, hi; };

// ---- settings ----
extern "C" unsigned sprint_reset_mode;                       // "mode"
extern "C" float sprint_reset_delay_value;                   // "delay": ticks after the hit
extern "C" float sprint_reset_duration_value;                // "duration": ticks the key is let go
extern "C" unsigned char sprint_reset_randomize_value;       // "randomize"
extern "C" unsigned char sprint_reset_wait_for_damage_value; // "wait_for_damage": only on the tick we take damage
extern "C" unsigned char sprint_reset_require_weapon_value;  // "require_weapon"

// ---- module state ----
enum SprintResetStage {
    STAGE_IDLE = 0,          // waiting for a hit
    STAGE_WAITING = 1,       // hit seen, waiting "delay"
    STAGE_KEY_RELEASED = 2,  // key let go, waiting "duration"
};
extern "C" int sprint_reset_stage;
extern "C" float sprint_reset_countdown;           // ticks left in the current stage
extern "C" double sprint_reset_time_last_own_hit;  // time of our last hit (seconds)
extern "C" unsigned char sprint_reset_hit_seen;    // a hit was seen and not handled yet
extern "C" int sprint_reset_ticks_since_hurt;
extern "C" char sprint_reset_rng_states[], sprint_reset_random_walk_1[], sprint_reset_random_walk_2[];   // random walk states

// ---- game / engine functions ----
FN(double, time_now, ());
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));            // 10 on the tick we are hit
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_sneak_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_key_17_or_8, (void *jvm, void *game_settings));   // the sprint key
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));   // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(void, java_call_entity_set_sprinting, (void *jvm, void *entity, bool on));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));        // next value of a random walk
FN(unsigned char, walk_should_change, (void *state, Range r, double p));
FN(float, random_rotation, (Range r));                           // random value in the range
extern "C" void sprint_reset_press_key(Ctx *ctx, unsigned char on);

extern "C" void sprint_reset_on_player_update(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1) {
        // Left the world: reset everything.
        if (sprint_reset_stage == STAGE_KEY_RELEASED)
            sprint_reset_press_key(ctx, 0);
        sprint_reset_stage = STAGE_IDLE;
        sprint_reset_hit_seen = 0;
        sprint_reset_countdown = 0.0f;
        return;
    }

    // Did we land a hit this tick? (at most one every half second)
    double now = time_now();
    if (!(now - sprint_reset_time_last_own_hit < 0.5) &&
        java_get_entity_ticks_existed(ctx->jvm, ctx->player) == java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player)) {
        sprint_reset_time_last_own_hit = now;
        sprint_reset_hit_seen = 1;
    }
    sprint_reset_ticks_since_hurt += 1;
    if (java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) == 10)
        sprint_reset_ticks_since_hurt = 0;

    // Count down the current stage.
    float countdown = sprint_reset_countdown;
    if (countdown > 0.0f) {
        countdown += -1.0f;
        sprint_reset_countdown = countdown;
        if (countdown > 0.0f) {
            if (sprint_reset_stage == STAGE_KEY_RELEASED)
                sprint_reset_press_key(ctx, 1);   // keep the key let go
            return;
        }
    }

    if (sprint_reset_stage == STAGE_KEY_RELEASED) {
        // "duration" is over: give the key back.
        sprint_reset_press_key(ctx, 0);
        sprint_reset_stage = STAGE_IDLE;
        return;
    }

    if (sprint_reset_stage == STAGE_WAITING) {
        // "delay" is over: let go of the key for "duration" ticks.
        sprint_reset_press_key(ctx, 1);
        sprint_reset_stage = STAGE_KEY_RELEASED;
        sprint_reset_hit_seen = 0;
        float duration = sprint_reset_duration_value;
        if (sprint_reset_randomize_value == 1) {
            float shorter = walk_update(sprint_reset_random_walk_1, rot_make_floats(0.0f, 1.0f), 30.0);
            float reduced = duration - shorter;
            duration = 0.0f > reduced ? 0.0f : reduced;
            // Now and then (5-10% of the time), 1-2 ticks longer.
            if (walk_should_change(sprint_reset_random_walk_2, rot_make_floats(0.05f, 0.1f), 30.0))
                duration += random_rotation(rot_make_floats(1.0f, 2.0f));
        }
        sprint_reset_countdown = duration + sprint_reset_countdown;
        return;
    }

    // Idle: after a hit, start waiting "delay" ticks.
    float delay = sprint_reset_delay_value;
    if (sprint_reset_randomize_value == 1)
        delay += walk_update(sprint_reset_rng_states, rot_make_floats(0.0f, 2.0f), 5.0);
    if (sprint_reset_hit_seen == 1) {
        if (sprint_reset_wait_for_damage_value == 1 && sprint_reset_ticks_since_hurt != 0)
            return;
        if (sprint_reset_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
            return;
        sprint_reset_countdown = delay + sprint_reset_countdown;
        sprint_reset_stage = STAGE_WAITING;
    }
}

// on = 1: do the reset (let go of the key / press sneak). on = 0: give the key back.
extern "C" void sprint_reset_press_key(Ctx *c, unsigned char on) {
    void *settings = get_game_settings(c->jvm, c->minecraft);
    void *key;
    if (sprint_reset_mode == 2)
        key = java_get_game_settings_key_17_or_8(c->jvm, settings);   // sprint key
    else if (sprint_reset_mode == 1)
        key = java_get_game_settings_sneak_key(c->jvm, settings);
    else
        key = java_get_game_settings_forward_key(c->jvm, settings);

    if (sprint_reset_mode == 2)
        java_call_entity_set_sprinting(c->jvm, c->player, on != 1);

    // Doing the reset: sneak is pressed, the other keys are let go.
    // Giving it back: the key goes back to what the player physically holds.
    unsigned char pressed = on ? (sprint_reset_mode == 1) : java_get_key_binding_key_code(c->jvm, key);
    java_set_key_binding_pressed(c->jvm, key, pressed);
}
