// Auto Block - block with your sword automatically while fighting.
//
// While you hold a sword and the conditions are met, the module holds the use key so you
// block, lets go after "max_hold_time" ticks so you can hit, and blocks again when a target
// is in range. After letting go it can "lag" (hold outgoing packets) for "lag_duration"
// ticks, with "lag_chance". "fake_block_animation" makes the sword look blocked on your
// screen even when it is not.
//
// Callbacks (descriptor slots +0x48 / +0x78 / +0x98 / +0x128 / +0x168):
//   auto_block_on_tick         every tick: block / unblock
//   auto_block_on_packet_sent  every outgoing packet: hold it while lagging
//   auto_block_on_toggle       module switched off: give the use key back
//   auto_block_on_attack       before an attack
//   auto_block_on_late_tick    every tick: the fake block animation
#include "slinky/engine.h"

struct AttackHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10
    unsigned char cancel;   // +0x11 set to 1 to cancel the attack
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct PktHook { char padding_00[0x24]; int verdict; };   // +0x24 set to 1 to hold the packet
struct Mouse { unsigned char padding_00, lmb, rmb; };      // Ctx::key_state_table: +1 left, +2 right button held

// ---- settings ----
extern "C" float auto_block_max_hold_time_value;                // "max_hold_time": ticks to block before letting go
extern "C" float auto_block_lag_chance_value;                   // "lag_chance"
extern "C" float auto_block_lag_duration_value;                 // "lag_duration": ticks
extern "C" float auto_block_max_hurt_time_value;                // "max_hurt_time": don't block later than this after being hit
extern "C" unsigned char auto_block_require_left_mouse_pressed_value;    // "require_left_mouse_pressed"
extern "C" unsigned char auto_block_require_right_mouse_pressed_value;   // "require_right_mouse_pressed"
extern "C" unsigned char auto_block_require_damage_value;       // "require_damage": only after being hit
extern "C" unsigned char auto_block_fake_block_animation;       // "fake_block_animation"
extern "C" unsigned char auto_block_fake_block_animation_require_target_value;   // "fake_block_animation_require_target"
extern "C" float auto_block_range_value;                        // "range": target distance to block at
extern "C" unsigned char auto_block_lag_flush_attack_value;     // "lag_flush_attack": stop lagging when we hit
extern "C" unsigned char auto_block_lag_reblock_value;          // "lag_reblock": block again right after lagging

// ---- module state ----
extern "C" char auto_block_module_descriptor[];
extern "C" unsigned char auto_block_blocking;            // we are blocking
extern "C" unsigned char auto_block_use_key_forced;      // we control the use key
extern "C" int auto_block_re_block_cooldown;             // ticks before blocking again
extern "C" int auto_block_ticks_held;                    // ticks blocked so far
extern "C" unsigned auto_block_ticks_since_target_hit;
extern "C" int auto_block_last_target_id;
extern "C" unsigned char auto_block_right_mouse_required;   // "require_right_mouse_pressed" is on (this tick)
extern "C" int auto_block_lag_ticks_left;                // > 0 while lagging (read by the network thread)
extern "C" unsigned char auto_block_fake_animation_this_tick;
extern "C" unsigned char auto_block_animation_started;
extern "C" int auto_block_animation_counter;             // item use count shown for the fake animation

// ---- game / engine functions ----
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void, java_set_entity_player_item_in_use_count, (void *jvm, void *entity_player, int n));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));   // 10 on the tick of being hit
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, item_sword, (void *jvm, void *item));                                  // is it a sword?
FN(unsigned char, timer_range_is_delay_pending, ());                                     // Timer Range is busy
FN(unsigned char, random_chance, (float chance));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));       // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));        // = one right click
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));

// Round a tick count randomly: 2.3 gives 3 with a 30% chance, else 2.
static int random_round(float value) {
    int whole = (int)value;
    return random_chance(value - (float)(int)value) + whole;
}

static void *use_key(Ctx *ctx) {
    return java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
}

static void stop_lagging() {
    __atomic_exchange_n(&auto_block_lag_ticks_left, 0, __ATOMIC_SEQ_CST);
}

static bool target_in_range(Ctx *ctx) {
    CombatTarget *target = (CombatTarget *)ctx->target;
    return target->ent && (double)(auto_block_range_value * auto_block_range_value) > target->dist2;
}

extern "C" void auto_block_on_tick(Hook *hook) {
    auto_block_fake_animation_this_tick = 0;
    Ctx *ctx = hook->ctx;

    // End last tick's fake animation.
    if (auto_block_animation_started == 1) {
        if (ctx->player && !java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
            java_set_entity_player_item_in_use_count(ctx->jvm, ctx->player, 0);
        auto_block_animation_started = 0;
    }
    if (auto_block_re_block_cooldown != 0)
        auto_block_re_block_cooldown -= 1;

    if (!ctx->in_game) {
        if (auto_block_use_key_forced == 1) {
            void *key = use_key(ctx);
            java_set_key_binding_pressed(ctx->jvm, key, java_get_key_binding_key_code(ctx->jvm, key));
            auto_block_use_key_forced = 0;
        }
        auto_block_right_mouse_required = 0;
        stop_lagging();
        return;
    }

    int hurt_time = java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player);
    unsigned char holding_sword = item_sword(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player));
    auto_block_right_mouse_required = 0;

    // Should the use key be ours this tick?
    unsigned char conditions_ok = 0;   // sword + mouse conditions (for the fake animation)
    unsigned char key_forced = 0;      // we press the use key this tick
    unsigned char fake_animation = 0;
    bool give_key_back;
    if (!holding_sword) {
        give_key_back = auto_block_use_key_forced == 1;   // only if we had it
        if (!give_key_back)
            goto bookkeeping;
    } else if (auto_block_require_left_mouse_pressed_value == 1 && !((Mouse *)ctx->key_state_table)->lmb) {
        give_key_back = true;
    } else if (timer_range_is_delay_pending()) {
        give_key_back = true;
    } else {
        unsigned char need_rmb = auto_block_require_right_mouse_pressed_value;
        auto_block_right_mouse_required = need_rmb;
        if (need_rmb != ((Mouse *)ctx->key_state_table)->rmb) {
            give_key_back = true;
        } else {
            conditions_ok = 1;
            // Blocking only right after being hit, and not too late.
            if (hurt_time == 0 && auto_block_require_damage_value)
                give_key_back = true;
            else if (hurt_time > random_round(auto_block_max_hurt_time_value))
                give_key_back = true;
            else
                give_key_back = false;
        }
    }

    if (give_key_back) {
        // The use key goes back to the player (or stays up while right mouse is required).
        void *key = use_key(ctx);
        java_set_key_binding_pressed(ctx->jvm, key, auto_block_right_mouse_required ? 0 : java_get_key_binding_key_code(ctx->jvm, key));
        key_forced = 0;
    } else {
        // Ours: pressed while blocking, up otherwise.
        unsigned char blocking = auto_block_blocking;
        void *key = use_key(ctx);
        java_set_key_binding_pressed(ctx->jvm, key, blocking);
        key_forced = 1;
        if (blocking == 1)
            java_increment_key_binding_press_time(ctx->jvm, key);
    }
    auto_block_use_key_forced = key_forced;
    if (conditions_ok && auto_block_fake_block_animation) {
        fake_animation = 1;
        if (auto_block_fake_block_animation_require_target_value == 1)
            fake_animation = target_in_range(ctx);
    }

bookkeeping:
    auto_block_fake_animation_this_tick = fake_animation;
    if (hurt_time == 10)
        auto_block_re_block_cooldown = 0;   // just got hit: may block again at once

    // Did we hit the target (or get a new one)?
    unsigned ticks = auto_block_ticks_since_target_hit + 1;
    auto_block_ticks_since_target_hit = ticks;
    unsigned char hit_target = 0;
    if (ticks >= 10) {
        hit_target = java_get_entity_ticks_existed(ctx->jvm, ctx->player) == java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player) + 1;
        if (hit_target)
            auto_block_ticks_since_target_hit = 0;
    }
    void *target = ((CombatTarget *)ctx->target)->ent;
    int target_id = target ? java_get_entity_entity_id(ctx->jvm, target) : 0;
    if (target_id != auto_block_last_target_id) {
        auto_block_last_target_id = target_id;
        auto_block_ticks_since_target_hit = 0;
        hit_target = 1;
    }

    // Count the lag down (or end it on a hit with "lag_flush_attack").
    if (__atomic_load_n(&auto_block_lag_ticks_left, __ATOMIC_SEQ_CST) != 0) {
        if ((hit_target & auto_block_lag_flush_attack_value) == 1)
            __atomic_exchange_n(&auto_block_lag_ticks_left, 0, __ATOMIC_SEQ_CST);
        else
            __atomic_fetch_sub(&auto_block_lag_ticks_left, 1, __ATOMIC_SEQ_CST);
        if (auto_block_lag_reblock_value == 1 && __atomic_load_n(&auto_block_lag_ticks_left, __ATOMIC_SEQ_CST) == 0)
            auto_block_re_block_cooldown = 0;   // lag over: block again at once
    }

    if (auto_block_blocking) {
        // Blocking: let go after "max_hold_time" ticks (randomly rounded).
        int held = auto_block_ticks_held + 1;
        float max_hold = auto_block_max_hold_time_value;
        int whole = (int)max_hold;
        auto_block_ticks_held = held;
        int limit = random_chance(max_hold - (float)(int)max_hold) + whole;
        unsigned char keep_holding = held < limit;
        if (key_forced & keep_holding)
            return;
        java_set_key_binding_pressed(ctx->jvm, use_key(ctx), 0);
        auto_block_use_key_forced = 1;
        if (random_chance(auto_block_lag_chance_value))
            __atomic_exchange_n(&auto_block_lag_ticks_left, random_round(auto_block_lag_duration_value), __ATOMIC_SEQ_CST);
        auto_block_blocking = 0;
        return;
    }

    // Not blocking: block again when allowed and a target is in range.
    if (!(key_forced & (auto_block_re_block_cooldown == 0)))
        return;
    if (!target_in_range(ctx))
        return;
    void *key = use_key(ctx);
    java_set_key_binding_pressed(ctx->jvm, key, 1);
    java_increment_key_binding_press_time(ctx->jvm, key);
    auto_block_use_key_forced = 1;
    auto_block_blocking = 1;
    auto_block_re_block_cooldown = 10;
    auto_block_ticks_held = 0;
    stop_lagging();
}

extern "C" void auto_block_on_packet_sent(PktHook *hook) {
    if (__atomic_load_n(&auto_block_lag_ticks_left, __ATOMIC_SEQ_CST) != 0)
        hook->verdict = 1;
}

// On disable: stop blocking and give the use key back.
extern "C" void auto_block_on_toggle(EvHook *hook) {
    if (hook->ev != auto_block_module_descriptor || hook->on || !(auto_block_blocking & 1))
        return;
    if (auto_block_use_key_forced == 1) {
        Ctx *ctx = hook->ctx;
        void *key = use_key(ctx);
        java_set_key_binding_pressed(ctx->jvm, key, java_get_key_binding_key_code(ctx->jvm, key));
        auto_block_use_key_forced = 0;
    }
    auto_block_blocking = 0;
    stop_lagging();
}

extern "C" void auto_block_on_attack(AttackHook *hook) {
    if (hook->phase != 0)
        return;
    void *jvm = hook->ctx->jvm;
    if (!item_sword(jvm, item_held_hotbar_slot(jvm, hook->ctx->player)))
        stop_lagging();
    // With "require_right_mouse_pressed", don't attack while not blocking.
    if (auto_block_right_mouse_required == 1 && !(auto_block_blocking & 1))
        hook->cancel = 1;
}

// Fake block animation: count the item use counter down so the sword is drawn blocking.
extern "C" void auto_block_on_late_tick(Hook *hook) {
    if (!auto_block_fake_animation_this_tick) {
        auto_block_animation_counter = 72000;   // a sword's full use duration
        return;
    }
    Ctx *ctx = hook->ctx;
    if (ctx->player) {
        auto_block_animation_started = 1;
        int count = auto_block_animation_counter - 1;
        auto_block_animation_counter = count;
        java_set_entity_player_item_in_use_count(ctx->jvm, ctx->player, count);
    }
}
