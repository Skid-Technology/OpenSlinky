// Hit Select - skip bad hits so you win more trades.
//
// Not every click should hit: hitting at the wrong moment gives the other player the
// first hit back, or wastes a combo. This module cancels some of your attacks, based on
// timing ("pause_dur", "trades_delay", ...) and "cancel_rate".
//
// Two modes:
//   0: wait until the target can take damage again, and don't trade hits while being comboed
//   1: prefer hits while falling (critical hits), cancel ones that would reduce your knockback
//
// Callbacks (descriptor slots +0x48 / +0x128 / +0x158):
//   hit_select_on_tick           every tick: count calm ticks (not hurt)
//   hit_select_on_attack         every attack: allow or cancel it (decided once per tick)
//   hit_select_on_player_update  every player tick: track when the target was last hit
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };

struct AttackHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10 1 = the attack is about to happen
    unsigned char cancel;   // +0x11 set to 1 to cancel it
};

// ---- settings ----
extern "C" unsigned hit_select_mode;                        // "mode"
extern "C" float hit_select_cancel_rate_value;              // "cancel_rate": chance to cancel a bad hit
extern "C" float hit_select_cancel_rate_air_value;          // "cancel_rate_air": chance to cancel a click on nothing
extern "C" float hit_select_pause_dur_value;                // "pause_dur": ticks to wait after the target was hit
extern "C" float hit_select_wait_for_damage_dur_value;      // "wait_for_damage_dur"
extern "C" float hit_select_trades_delay_value;             // "trades_delay": extra wait while we are being hit
extern "C" unsigned char hit_select_use_server_time_value;  // "use_server_time": use the target's hurt time
extern "C" unsigned char hit_select_fake_swing_value;       // "fake_swing": swing the arm on cancelled hits
extern "C" unsigned char hit_select_crits_allow_reduce_value;   // "crits_allow_reduce"
extern "C" unsigned char hit_select_require_damage_value;   // "require_damage"
extern "C" unsigned char hit_select_require_sprint_value;   // "require_sprint"
extern "C" unsigned char hit_select_require_weapon_value;   // "require_weapon"

// ---- module state ----
extern "C" unsigned char hit_select_decided_this_tick;
extern "C" unsigned char hit_select_cancel;                 // decision for this tick
extern "C" int hit_select_calm_ticks;                       // ticks since we were last hurt (on the ground)
extern "C" unsigned hit_select_ticks_since_target_was_hit;
extern "C" unsigned hit_select_wait_damage_counter;
extern "C" unsigned char hit_select_target_just_got_hit;
extern "C" int hit_select_last_target_id;

// ---- game / engine functions ----
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));   // 10 on the tick of being hit
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void, java_jni_call_nonvirtual_void_method_entity_living_base_swing_item, (void *jvm, void *object));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 0 = nothing, 1 = block, 2 = entity
FN(void *, java_get_moving_object_position_entity_hit, (void *jvm, void *moving_object_position));
FN(unsigned char, random_chance, (float chance));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(float, java_get_entity_fall_distance, (void *jvm, void *entity));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(double, libc_floor, (double v));
FN(void *, java_call_variant, (void *jvm, void *world, int x, int y, int z));               // block at x, y, z
FN(int, get_block_id, (void *jvm, void *blk));
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack

// Round a tick count randomly: 2.3 gives 3 with a 30% chance, else 2.
static unsigned random_round(float value) {
    unsigned whole = (unsigned)(long long)value;
    return random_chance(value - (float)(unsigned long long)whole) + whole;
}

extern "C" void hit_select_on_tick(Hook *hook) {
    hit_select_decided_this_tick = 0;
    Ctx *ctx = hook->ctx;
    int calm = 0;
    if (ctx->in_game == 1 && java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) == 0)
        calm = hit_select_calm_ticks + 1;
    hit_select_calm_ticks = calm;
}

enum Decision {
    ALLOW,    // let the hit through
    CANCEL,   // cancel it
    ROLL,     // too early: cancel with "cancel_rate" chance, unless the pause is over
};

// Decide about this attack. For ROLL, *pause_ticks is how long to wait after the target was hit.
static Decision decide(Ctx *ctx, unsigned *pause_ticks) {
    if (hit_select_require_sprint_value == 1 && !java_call_entity_is_sprinting(ctx->jvm, ctx->player))
        return ALLOW;
    if (hit_select_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return ALLOW;

    // What is under the crosshair?
    bool no_entity = true;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (hit) {
        switch (java_get_moving_object_position_type_ordinal(ctx->jvm, hit)) {
        case 2:   // an entity
            no_entity = java_get_moving_object_position_entity_hit(ctx->jvm, hit) == nullptr;
            break;
        case 1:   // a block: always allow (mining)
            return ALLOW;
        case 0:   // nothing
            if (random_chance(hit_select_cancel_rate_air_value))
                return CANCEL;
            break;
        }
    }

    unsigned pause = random_round(hit_select_pause_dur_value);
    *pause_ticks = pause;

    // Going up (jumping) while having a target.
    unsigned char rising = 1;
    unsigned char on_ground = java_get_entity_on_ground(ctx->jvm, ctx->player);
    if (on_ground)
        rising = java_get_entity_motion_x(ctx->jvm, ctx->player).y > 0.0;
    rising &= ((CombatTarget *)ctx->target)->valid;

    if (hit_select_mode == 1) {
        // Outside the 2..8 tick window after the target was hit (or using server time).
        unsigned char outside_window = (hit_select_ticks_since_target_was_hit - 9 < (unsigned)-7) | hit_select_use_server_time_value;
        unsigned char falling;
        if (on_ground) {
            falling = 0;
        } else {
            falling = java_get_entity_fall_distance(ctx->jvm, ctx->player) > 0.0f;
            if (falling & outside_window)
                return ALLOW;   // a critical hit
        }
        if (rising && (!outside_window || hit_select_crits_allow_reduce_value))
            return ALLOW;

        // A block at head height (we can't jump): allow.
        Vec3 pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
        if (java_variant(ctx->jvm))
            pos = vec3_add_eye_height_sneak(pos);
        int x = (int)libc_floor(pos.x);
        int y = (int)libc_floor(pos.y);
        int z = (int)libc_floor(pos.z);
        if (get_block_id(ctx->jvm, java_call_variant(ctx->jvm, ctx->world, x, y, z)) != 0)
            return ALLOW;

        if (on_ground | falling)
            return ROLL;
        if (hit_select_require_damage_value != 1 || java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) != 0)
            return CANCEL;   // rising in the air: wait for the fall
        return ROLL;
    }

    if (hit_select_mode != 0 || rising)
        return ALLOW;

    // Mode 0. Right after the target was hit, wait "wait_for_damage_dur" ticks.
    if (hit_select_ticks_since_target_was_hit < 15) {
        hit_select_wait_damage_counter = 0;
    } else if (!no_entity) {
        unsigned waited = hit_select_wait_damage_counter + 1;
        hit_select_wait_damage_counter = waited;
        if (waited <= random_round(hit_select_wait_for_damage_dur_value))
            return CANCEL;
    }
    // Being hit ourselves recently: wait longer before trading.
    if ((unsigned)hit_select_calm_ticks <= 9)
        *pause_ticks = pause + random_round(hit_select_trades_delay_value);
    if (hit_select_target_just_got_hit == 1) {
        hit_select_target_just_got_hit = 0;
        return ALLOW;
    }
    return ROLL;
}

// Cancel the attack (and optionally swing the arm anyway, so it looks normal).
static void cancel_attack(AttackHook *hook, Ctx *ctx) {
    hook->cancel = 1;
    if (hit_select_fake_swing_value == 1 && ctx->player && !java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
        java_jni_call_nonvirtual_void_method_entity_living_base_swing_item(ctx->jvm, ctx->player);
}

extern "C" void hit_select_on_attack(AttackHook *hook) {
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;

    // Already decided this tick: do the same again.
    if (hit_select_decided_this_tick == 1) {
        if (hit_select_cancel)
            cancel_attack(hook, ctx);
        return;
    }

    unsigned pause_ticks = 0;
    Decision decision = decide(ctx, &pause_ticks);
    if (decision == ROLL) {
        if (pause_ticks <= hit_select_ticks_since_target_was_hit + 1) {
            decision = ALLOW;   // the pause is over
        } else {
            unsigned char cancel = random_chance(hit_select_cancel_rate_value);
            hit_select_cancel = cancel;
            hit_select_decided_this_tick = 1;
            if (cancel)
                cancel_attack(hook, ctx);
            return;
        }
    }

    hit_select_decided_this_tick = 1;
    if (decision == ALLOW) {
        hit_select_cancel = 0;
        return;
    }
    hit_select_cancel = 1;
    cancel_attack(hook, ctx);
}

// Every player tick: count the ticks since the target was last hit.
extern "C" void hit_select_on_player_update(Hook *hook) {
    Ctx *ctx = hook->ctx;
    void *target = ((CombatTarget *)ctx->target)->ent;

    if (hit_select_use_server_time_value == 1) {
        // Server time: the target's hurt time is 10 on the tick it was hit.
        if (target && java_get_entity_living_base_hurt_time(ctx->jvm, target) == 10) {
            hit_select_ticks_since_target_was_hit = 0;
            hit_select_target_just_got_hit = 1;
            return;
        }
        hit_select_ticks_since_target_was_hit += 1;
        return;
    }

    // Our own time: the tick we last attacked.
    int target_id = target ? java_get_entity_entity_id(ctx->jvm, target) : 0;
    if (target_id == hit_select_last_target_id) {
        unsigned ticks = hit_select_ticks_since_target_was_hit + 1;
        hit_select_ticks_since_target_was_hit = ticks;
        if (ticks < 10)
            return;
    } else {
        // New target: "long ago".
        hit_select_last_target_id = target_id;
        hit_select_ticks_since_target_was_hit = 1001;
    }
    if (java_get_entity_ticks_existed(ctx->jvm, ctx->player) == java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player))
        hit_select_ticks_since_target_was_hit = 0;
}
