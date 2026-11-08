// Auto Rod - throw your fishing rod (or snowballs / eggs) at your target, then switch back.
//
// When switched on (by its keybind), the module switches to the rod, throws it, waits until
// the bobber should have reached the target, reels it in by switching back to your weapon,
// and switches itself off. With "allow_throwables" snowballs and eggs are preferred.
// Delays are randomised with "randomize_delays".
//
// Callbacks:
//   auto_rod_on_input                 keybind pressed while running: cancel
//   auto_rod_on_toggle                switched on: start over
//   auto_rod_on_attack                left click with the rod: don't swing twice
//   auto_rod_randomised_throw_delay   helper: delay between steps
//   auto_rod_randomised_rod_pull      helper: delay between throws (repeat mode)
//   auto_rod_ticks_bobber_needs       helper: ticks until the bobber reaches the target
//   auto_rod_on_tick                  every tick: the next step
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct AABB { Vec3 lo, hi; };
struct Range { float lo, hi; };

struct InHook {
    char padding_00[8];
    void *mod;               // +0x08 module whose keybind was pressed
    unsigned char use;       // +0x10 set to 1: we used the key press
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct AttackHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10 0 = before the click
    unsigned char cancel;   // +0x11 set to 1 to cancel it
};

// ---- settings ----
extern "C" float auto_rod_throw_delay;                       // "delay": ticks between steps
extern "C" unsigned char auto_rod_randomise;                 // "randomize_delays"
extern "C" float auto_rod_pull_speed;                        // "retract_speed": throws per second (repeat mode)
extern "C" float auto_rod_projectile_speed;                  // "throw_speed": blocks per tick
extern "C" unsigned char auto_rod_abort_when_aiming_at_entity;   // "require_not_mouse_over"
extern "C" unsigned char auto_rod_prefer_snowballs_eggs;     // "allow_throwables"
extern "C" unsigned char auto_rod_switch_best_weapon_afterwards;   // "always_switch_to_weapon"
// Engine flag: 1 = keep repeating until the keybind is pressed again.
extern "C" unsigned char timer_range_min_dist_setting_field_0x58;

// ---- module state ----
extern "C" char auto_rod_module_descriptor[];
enum RodStage {
    STAGE_START = 0,        // find the rod and take the hotbar
    STAGE_SWITCH = 1,       // switch to it
    STAGE_THROW = 2,
    STAGE_WAIT = 3,         // wait for the bobber to reach the target
    STAGE_REEL = 4,         // (same slot to go back to) switch away for a moment to reel in
    STAGE_SWITCH_BACK = 5,
    STAGE_DONE = 6,
    STAGE_CANCEL = 7,
};
extern "C" int auto_rod_stage;
extern "C" unsigned char auto_rod_input_taken;     // the keybind was pressed (repeat mode): stop
extern "C" unsigned char auto_rod_rod_swing_seen;
extern "C" int auto_rod_slot_switch_back;          // slot to go back to, -1 = none
extern "C" int auto_rod_rod_slot;
extern "C" float auto_rod_step_delay;              // ticks until the next step
extern "C" int auto_rod_wait_ticks;
extern "C" unsigned char auto_rod_using_fishing_rod;   // 1 = rod, 0 = snowball / egg
extern "C" char auto_rod_rng_states[], auto_rod_rng_states_second[], auto_rod_rng_states_third[], auto_rod_rng_states_fourth[];   // random walk states

// ---- game / engine functions ----
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void, java_set_inventory_player_current_item, (void *jvm, void *inventory_player, int slot));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void, release_owner, (void *timer));                               // give control of the hotbar back
FN(unsigned char, claim_release_hotbar_module, (void *timer));        // ask for control of the hotbar
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 2 = an entity
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, weapons_find_first_weapon_slot_entry, (Ctx *ctx, void *slot));   // see weapons.cpp
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void, java_set_key_binding_press_time, (void *jvm, void *key_binding, int n));   // 1 = one right click
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height, (Vec3 v));
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));   // server position
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(AABB, aabb_make, (Vec3 lo, Vec3 hi));
FN(Vec3, aabb_clamp_point, (AABB box, Vec3 p));                       // closest point of the box
FN(double, vec3_distance, (Vec3 a, Vec3 b));
FN(float, libc_ceilf, (float v));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));            // next value of a random walk
FN(unsigned char, walk_should_change, (void *state, Range r, double p));   // random yes/no
FN(float, random_rotation, (Range r));                                // random value in the range
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));

enum { ITEM_SNOWBALL = 332, ITEM_EGG = 344, ITEM_FISHING_ROD = 346 };

static bool repeat_mode() { return timer_range_min_dist_setting_field_0x58 == 1; }

extern "C" void auto_rod_on_input(InHook *hook) {
    if (auto_rod_input_taken != 0 || hook->mod != auto_rod_module_descriptor)
        return;
    if (repeat_mode()) {
        auto_rod_input_taken = 1;   // stop repeating
    } else {
        int stage = auto_rod_stage;
        if (stage == STAGE_DONE)
            return;
        if (stage == STAGE_WAIT)
            auto_rod_stage = STAGE_CANCEL;
    }
    hook->use = 1;
}

extern "C" void auto_rod_on_toggle(EvHook *hook) {
    if (hook->ev != auto_rod_module_descriptor || hook->on != 1)
        return;
    auto_rod_stage = STAGE_START;
}

// Clicking with the rod: let the first swing through, cancel the others.
extern "C" void auto_rod_on_attack(AttackHook *hook) {
    if (hook->phase != 0)
        return;
    Ctx *ctx = hook->ctx;
    void *held = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (held && java_call_static_item_get_id_from_item(ctx->jvm, held) == ITEM_FISHING_ROD) {
        if (auto_rod_rod_swing_seen)
            hook->cancel = 1;
        else
            auto_rod_rod_swing_seen = 1;
    }
}

// "delay", randomly up to 0.5 ticks shorter, and now and then (1-3%) 0.5-1 tick longer.
extern "C" float auto_rod_randomised_throw_delay() {
    float delay = auto_rod_throw_delay;
    if (auto_rod_randomise == 1) {
        float shorter = delay - walk_update(auto_rod_rng_states, rot_make_floats(0.0f, 0.5f), 5.0);
        delay = 0.0f;
        delay = delay > shorter ? delay : shorter;
        if (walk_should_change(auto_rod_rng_states_second, rot_make_floats(0.01f, 0.03f), 30.0))
            delay += random_rotation(rot_make_floats(0.5f, 1.0f));
    }
    return delay;
}

// Ticks between throws: 20 / "retract_speed", randomly up to 2x longer, now and then 1-2 ticks more.
extern "C" float auto_rod_randomised_rod_pull() {
    float speed = auto_rod_pull_speed;
    if (!auto_rod_randomise)
        return 20.0f / speed;
    float factor = walk_update(auto_rod_rng_states_third, rot_make_floats(0.5f, 1.0f), 1.0);
    float ticks = 20.0f / (speed * factor);
    if (walk_should_change(auto_rod_rng_states_fourth, rot_make_floats(0.01f, 0.03f), 30.0))
        ticks += random_rotation(rot_make_floats(1.0f, 2.0f));
    return ticks;
}

// Ticks the bobber needs to reach the target's hitbox (10 blocks without a target).
extern "C" int auto_rod_ticks_bobber_needs(Ctx *ctx) {
    void *target = ((CombatTarget *)ctx->target)->ent;
    float ticks;
    if (!target) {
        ticks = 10.0f / auto_rod_projectile_speed;
    } else {
        Vec3 eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
        if (!java_variant(ctx->jvm))
            eye = vec3_add_eye_height(eye);
        // The target's hitbox: 0.6 wide, 1.8 tall.
        Vec3 pos = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, target);
        Vec3 lo = vec3_sub(pos, vec3_make(0.3, 0.0, 0.3));
        Vec3 hi = vec3_add(pos, vec3_make(0.3, 1.8, 0.3));
        Vec3 nearest = aabb_clamp_point(aabb_make(lo, hi), eye);
        ticks = (float)(vec3_distance(eye, nearest) / (double)auto_rod_projectile_speed);
    }
    return (int)libc_ceilf(ticks);
}

static void restore_slot(Ctx *ctx) {
    if (auto_rod_slot_switch_back != -1) {
        java_set_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player), auto_rod_slot_switch_back);
        auto_rod_slot_switch_back = -1;
    }
}

// The hotbar slot with the rod (or, with "allow_throwables", a snowball / egg), or -1.
// Sets auto_rod_using_fishing_rod.
static int find_throwable(Ctx *ctx, void *inventory) {
    auto_rod_using_fishing_rod = 0;
    int found = -1;
    for (int i = 0; i < 9; i++) {
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, i);
        if (!stack)
            continue;
        void *item = java_call_item_stack_get_item(ctx->jvm, stack);
        if (!item)
            continue;
        int item_id = java_call_static_item_get_id_from_item(ctx->jvm, item);
        if (item_id == ITEM_SNOWBALL || item_id == ITEM_EGG) {
            if (auto_rod_prefer_snowballs_eggs == 1) {
                auto_rod_using_fishing_rod = 0;
                return i;
            }
            continue;
        }
        if (item_id != ITEM_FISHING_ROD)
            continue;
        auto_rod_using_fishing_rod = 1;
        found = i;
        if (!auto_rod_prefer_snowballs_eggs)
            return i;   // (with throwables allowed, keep looking for one; the rod is the fallback)
    }
    return found;
}

// Done: in repeat mode start again, otherwise switch off.
static void finished(Ctx *ctx) {
    if (repeat_mode()) {
        auto_rod_stage = STAGE_START;
        return;
    }
    queue_module_toggle(ctx->module_manager, auto_rod_module_descriptor, 0);
}

// Throw: right click, then wait for the bobber (rod), the next throw (repeat mode), or switch back.
static void throw_item(Ctx *ctx) {
    java_set_key_binding_press_time(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)), 1);
    if (auto_rod_using_fishing_rod == 1) {
        auto_rod_stage = STAGE_WAIT;
        auto_rod_wait_ticks = auto_rod_ticks_bobber_needs(ctx);
        auto_rod_step_delay += auto_rod_randomised_throw_delay();
    } else if (repeat_mode()) {
        auto_rod_step_delay += auto_rod_randomised_rod_pull();
    } else {
        auto_rod_stage = STAGE_SWITCH_BACK;
        auto_rod_step_delay += auto_rod_randomised_throw_delay();
    }
}

extern "C" void auto_rod_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    if (auto_rod_input_taken) {
        restore_slot(ctx);
        release_owner(auto_rod_module_descriptor);
        queue_module_toggle(ctx->module_manager, auto_rod_module_descriptor, 0);
        auto_rod_input_taken = 0;
        return;
    }

    // Up to 3 steps per tick, while there is no delay.
    for (int step = 0; step < 3; step++) {
        if (auto_rod_abort_when_aiming_at_entity == 1) {
            // Aiming at an entity: just hit it instead.
            void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
            if (hit && java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 2) {
                restore_slot(ctx);
                release_owner(auto_rod_module_descriptor);
                auto_rod_stage = STAGE_DONE;
                finished(ctx);
                return;
            }
        }
        if (auto_rod_stage == STAGE_CANCEL) {
            restore_slot(ctx);
            release_owner(auto_rod_module_descriptor);
            auto_rod_stage = STAGE_START;
            return;
        }
        if (auto_rod_wait_ticks)
            auto_rod_wait_ticks -= 1;
        float delay = auto_rod_step_delay;
        if (delay > 0.0f) {
            delay += -1.0f;
            auto_rod_step_delay = delay;
            if (delay > 0.0f)
                break;
        }

        void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
        int current_slot = java_get_inventory_player_current_item(ctx->jvm, inventory);
        unsigned stage = auto_rod_stage;
        int rod_slot = auto_rod_rod_slot;
        if (stage >= STAGE_THROW && current_slot != rod_slot) {
            // The player switched slot: stop.
            release_owner(auto_rod_module_descriptor);
            auto_rod_stage = STAGE_DONE;
            finished(ctx);
            return;
        }

        switch (stage) {
        case STAGE_START: {
            int found = find_throwable(ctx, inventory);
            if (found == -1 || !claim_release_hotbar_module(auto_rod_module_descriptor)) {
                auto_rod_stage = STAGE_DONE;
                finished(ctx);
                return;
            }
            auto_rod_rod_slot = found;
            auto_rod_slot_switch_back = current_slot;
            if (auto_rod_switch_best_weapon_afterwards == 1) {
                int weapon_slot = weapons_find_first_weapon_slot_entry(ctx, inventory);
                if (weapon_slot != -1)
                    auto_rod_slot_switch_back = weapon_slot;
                current_slot = auto_rod_slot_switch_back;
            }
            // Already holding it: throw straight away, else switch first.
            auto_rod_stage = (auto_rod_rod_slot == current_slot) + STAGE_SWITCH;
            continue;
        }
        case STAGE_THROW: {
            int found = find_throwable(ctx, inventory);
            if (found == -1) {
                auto_rod_stage = STAGE_SWITCH_BACK;   // nothing left to throw
                return;
            }
            if (found == current_slot) {
                throw_item(ctx);
                goto check_done;
            }
            auto_rod_rod_slot = found;
            auto_rod_stage = STAGE_SWITCH;
            auto_rod_step_delay += auto_rod_randomised_throw_delay();
            continue;
        }
        case STAGE_SWITCH:
            java_set_inventory_player_current_item(ctx->jvm, inventory, rod_slot);
            auto_rod_stage = STAGE_THROW;
            auto_rod_rod_swing_seen = 0;
            auto_rod_step_delay += auto_rod_randomised_throw_delay();
            continue;
        case STAGE_WAIT:
            if (auto_rod_wait_ticks != 0)
                goto check_done;
            // Reel in: switching slot does that. Going back to the same slot needs a detour.
            auto_rod_stage = auto_rod_slot_switch_back == rod_slot ? STAGE_REEL : STAGE_SWITCH_BACK;
            auto_rod_step_delay += auto_rod_randomised_throw_delay();
            continue;
        case STAGE_REEL:
            java_set_inventory_player_current_item(ctx->jvm, inventory, (auto_rod_slot_switch_back + 1) % 9);
            auto_rod_stage = STAGE_SWITCH_BACK;
            return;
        case STAGE_SWITCH_BACK:
            auto_rod_stage = STAGE_DONE;
            restore_slot(ctx);
            release_owner(auto_rod_module_descriptor);
            auto_rod_step_delay += auto_rod_randomised_throw_delay();
            goto check_done;
        default:
            goto check_done;
        }
    }

check_done:
    if (auto_rod_stage == STAGE_DONE)
        finished(ctx);
}
