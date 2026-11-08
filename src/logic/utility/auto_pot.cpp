// Auto Pot - throw splash healing potions.
//
// When switched on (usually by its keybind when you are low), the module works out how many
// potions you need (each heals 8 health), switches to a splash potion in the hotbar, throws
// it, repeats, switches back and switches itself off. With "double_splash" all of them are
// thrown at once; otherwise pressing the keybind again throws another one.
//
// Callbacks (descriptor slots +0x48 / +0x88 / +0x98):
//   auto_pot_on_tick       every tick: the next step
//   auto_pot_on_input      keybind pressed again: one more potion
//   auto_pot_on_toggle     switched on: how many potions to throw
//   auto_pot_asm_r_pins    helper: a random delay between steps
#include "slinky/engine.h"

struct Range { float lo, hi; };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct InHook {
    char padding_00[8];
    void *mod;               // +0x08 module whose keybind was pressed
    unsigned char use;       // +0x10 set to 1: we used the key press
};

// ---- settings ----
extern "C" unsigned char auto_pot_use_non_health_value;   // "use_non_health": throw other splash potions too
extern "C" float auto_pot_delay_value;                    // "delay": ticks between steps
extern "C" unsigned char auto_pot_prevent_waste_value;    // "prevent_waste": not when at full health
extern "C" unsigned char auto_pot_double_splash_value;    // "double_splash": throw all needed at once

// ---- module state ----
extern "C" char auto_pot_module_descriptor[];
enum PotStage {
    STAGE_FIND_POTION = 0,
    STAGE_SWITCH = 1,
    STAGE_THROW = 2,
    STAGE_NEXT = 3,      // next potion, or switch back
    STAGE_DONE = 4,      // switch off
};
extern "C" int auto_pot_stage;
extern "C" int auto_pot_potion_slot;
extern "C" int auto_pot_original_slot;
extern "C" unsigned char auto_pot_keep_original_slot;   // throwing another one: remember the slot from the first
extern "C" float auto_pot_step_delay;                   // ticks until the next step
extern "C" int auto_pot_potions_left_throw;
extern "C" int auto_pot_extra_throws;                   // extra potions the keybind may still ask for
extern "C" char auto_pot_rng_states[], auto_pot_initial_stage[];   // random walk states

// ---- game / engine functions ----
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *item_stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned, java_get_item_stack_metadata, (void *jvm, void *stack));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void, java_set_inventory_player_current_item, (void *jvm, void *inventory_player, int slot));
FN(unsigned char, claim_release_hotbar_module, (void *timer));        // ask for control of the hotbar
FN(void, release_owner, (void *timer));                                // give control of the hotbar back
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));   // = one right click
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(float, java_call_entity_living_base_get_max_health, (void *jvm, void *entity_living_base));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));            // next value of a random walk
FN(unsigned char, walk_should_change, (void *state, Range r, double p));   // random yes/no
FN(float, random_rotation, (Range r));                                // random value in the range
extern "C" float auto_pot_asm_r_pins();

enum { ITEM_POTION = 373, POTION_SPLASH_FLAG = 0x4000, POTION_EFFECT_HEALING = 5 };

// The first hotbar slot with a splash potion we may throw, or 9 when there is none.
static int find_potion(Ctx *ctx, void *inventory) {
    int slot = 0;
    for (; slot < 9; slot++) {
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, slot);
        if (!stack)
            continue;
        void *item = java_call_item_stack_get_item(ctx->jvm, stack);
        if (!item || java_call_static_item_get_id_from_item(ctx->jvm, item) != ITEM_POTION)
            continue;
        unsigned damage = java_get_item_stack_metadata(ctx->jvm, stack);
        if (!(damage & POTION_SPLASH_FLAG))
            continue;
        if ((damage & 0xf) != POTION_EFFECT_HEALING && !auto_pot_use_non_health_value)
            continue;
        break;
    }
    return slot;
}

// Give up: switch off.
static void stop(Ctx *ctx) {
    auto_pot_stage = STAGE_DONE;
    queue_module_toggle(ctx->module_manager, auto_pot_module_descriptor, 0);
}

extern "C" void auto_pot_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;

    // Up to 4 steps per tick, while there is no delay.
    for (int step = 0; step < 4; step++) {
        float delay = auto_pot_step_delay;
        if (delay > 0.0f) {
            auto_pot_step_delay = delay + -1.0f;
            break;
        }
        if (!ctx->in_game) {
            auto_pot_keep_original_slot = 0;
            stop(ctx);
            return;
        }

        switch (auto_pot_stage) {
        case STAGE_FIND_POTION: {
            void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
            int slot = find_potion(ctx, inventory);
            if (slot == 9) {
                // None (left): switch back if this was another throw, else switch off.
                auto_pot_potions_left_throw = 0;
                int next = STAGE_DONE - auto_pot_keep_original_slot;
                auto_pot_stage = next;
                auto_pot_keep_original_slot = 0;
                if (next == STAGE_DONE)
                    queue_module_toggle(ctx->module_manager, auto_pot_module_descriptor, 0);
                return;
            }
            auto_pot_potion_slot = slot;
            int original;
            if (auto_pot_keep_original_slot == 1) {
                original = auto_pot_original_slot;
            } else {
                original = java_get_inventory_player_current_item(ctx->jvm, inventory);
                auto_pot_original_slot = original;
                slot = auto_pot_potion_slot;
            }
            auto_pot_keep_original_slot = 0;
            // Already holding it: throw straight away, else switch first.
            auto_pot_stage = (slot == original) + STAGE_SWITCH;
            continue;
        }
        case STAGE_SWITCH:
            if (!claim_release_hotbar_module(auto_pot_module_descriptor)) {
                stop(ctx);
                return;
            }
            java_set_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player), auto_pot_potion_slot);
            auto_pot_stage = STAGE_THROW;
            auto_pot_step_delay = auto_pot_asm_r_pins() + auto_pot_step_delay;
            continue;
        case STAGE_THROW:
            if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
                goto finished;   // wait for the next tick
            java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));
            auto_pot_potions_left_throw += -1;
            auto_pot_stage = STAGE_NEXT;
            if (auto_pot_potion_slot != auto_pot_original_slot)
                auto_pot_step_delay = (auto_pot_asm_r_pins() + 1.0f) + auto_pot_step_delay;
            continue;
        case STAGE_NEXT:
            if (auto_pot_potions_left_throw > 0) {
                auto_pot_keep_original_slot = 1;
                auto_pot_stage = STAGE_FIND_POTION;
            } else {
                java_set_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player), auto_pot_original_slot);
                auto_pot_stage = STAGE_DONE;
            }
            continue;
        case STAGE_DONE:
            release_owner(auto_pot_module_descriptor);
            goto finished;
        default:
            __builtin_unreachable();
        }
    }

finished:
    if (auto_pot_stage == STAGE_DONE)
        queue_module_toggle(ctx->module_manager, auto_pot_module_descriptor, 0);
}

// The keybind was pressed again: throw one more (if allowed).
extern "C" void auto_pot_on_input(InHook *hook) {
    if (hook->mod != auto_pot_module_descriptor || auto_pot_stage == STAGE_DONE)
        return;
    int extra = auto_pot_extra_throws;
    if (extra <= 0)
        return;
    hook->use = 1;
    auto_pot_potions_left_throw += 1;
    auto_pot_extra_throws = extra - 1;
}

// Switched on: how many potions? Switched off: reset.
extern "C" void auto_pot_on_toggle(EvHook *hook) {
    if (hook->ev != auto_pot_module_descriptor)
        return;
    if (hook->on != 1) {
        auto_pot_stage = STAGE_FIND_POTION;
        auto_pot_potions_left_throw = 0;
        return;
    }
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;

    // One potion heals 8 health.
    float health = java_call_entity_living_base_get_health(ctx->jvm, ctx->player);
    int needed = (int)(((double)(java_call_entity_living_base_get_max_health(ctx->jvm, ctx->player) - health) + 0.5) * 0.125);
    if (needed == 0) {
        needed = 1;
        if (auto_pot_prevent_waste_value == 1) {
            auto_pot_stage = STAGE_DONE;   // nearly full health: don't waste one
            return;
        }
    }
    if (auto_pot_double_splash_value == 1) {
        auto_pot_potions_left_throw = needed;
        auto_pot_extra_throws = 0;
    } else {
        auto_pot_potions_left_throw = 1;
        auto_pot_extra_throws = auto_pot_prevent_waste_value ? needed - 1 : 5;
    }
}

// "delay" minus up to 0.5 ticks at random (not below 0); now and then (1-3%) 0.5-1 tick more.
extern "C" float auto_pot_asm_r_pins() {
    float delay = 0.0f;
    float shorter = auto_pot_delay_value - walk_update(auto_pot_rng_states, rot_make_floats(0.0f, 0.5f), 5.0);
    delay = delay > shorter ? delay : shorter;
    if (walk_should_change(auto_pot_initial_stage, rot_make_floats(0.01f, 0.03f), 30.0))
        delay += random_rotation(rot_make_floats(0.5f, 1.0f));
    return delay;
}
