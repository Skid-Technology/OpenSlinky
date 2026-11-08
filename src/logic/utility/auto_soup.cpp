// Auto Soup - eat soup automatically when your health is low.
//
// When health drops to "single_soup_max_hp", the module switches to a soup in the hotbar,
// eats it (soup is eaten instantly), optionally drops the empty bowl, and switches back.
// At "double_soup_max_hp" or lower it eats a second soup straight away. Every step waits a
// random delay ("delay_before" / "delay_between" / "delay_after", scaled by "randomization").
//
// Callbacks:
//   auto_soup_input_hook     blocks the player's own hotbar input while eating
//   auto_soup_enable         switched on: start over
//   auto_soup_asm_pins_keep  helper: one random delay
//   auto_soup_on_tick        every tick: the next step
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Range { float lo, hi; };

struct InHook {
    char padding_00[8];
    void *mod;               // +0x08 module asking for the hotbar
    unsigned char use;       // +0x10 set to 1: block the player's input
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" float auto_soup_single_soup_max_hp;              // "single_soup_max_hp": eat at this health or lower
extern "C" float auto_soup_double_soup_max_hp;              // "double_soup_max_hp": eat two at this health or lower
extern "C" float auto_soup_delay_scale;                     // "randomization"
extern "C" float auto_soup_delays_refill_health;            // "delay_before"
extern "C" float auto_soup_delays_refill_health_second;     // "delay_between"
extern "C" float auto_soup_delays_refill_health_third;      // "delay_after"
extern "C" unsigned char auto_soup_drop_bowls_value;        // "drop_bowls"
extern "C" unsigned char auto_soup_override_item;           // "override_item": use "allowed_items" instead of soup
extern "C" char auto_soup_custom_item_set[];                // "allowed_items"

// ---- module state ----
extern "C" char auto_soup_module_descriptor[];
enum SoupStage {
    STAGE_CHECK_HEALTH = 0,
    STAGE_FIND_SOUP = 1,
    STAGE_SWITCH = 2,
    STAGE_EAT = 3,
    STAGE_DROP_BOWL = 4,
    STAGE_SWITCH_BACK = 5,   // (or eat a second one)
    STAGE_COOL_DOWN = 6,
};
extern "C" int auto_soup_stage;
extern "C" unsigned char auto_soup_busy;               // eating the second soup
extern "C" unsigned char auto_soup_input_consumed;     // the player pressed a hotbar key while we were eating
extern "C" float auto_soup_step_delay;                 // ticks until the next step
extern "C" int auto_soup_soup_slot;
extern "C" int auto_soup_original_slot;
extern "C" unsigned char auto_soup_eat_another_one;
extern "C" unsigned char auto_soup_item_leaves_bowl;
extern "C" char auto_soup_rng_states[], auto_soup_random_walk_1[], auto_soup_random_walk_2[];   // random states

// ---- game / engine functions ----
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inv));     // the item array
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned char, id_vector_contains, (void *set, int id));
FN(unsigned char, claim_release_hotbar_module, (void *timer));                   // ask for control of the hotbar
FN(void, release_owner, (void *timer));                                           // give control of the hotbar back
FN(void, java_set_inventory_player_current_item, (void *jvm, void *inventory_player, int slot));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_key_8_or_9, (void *jvm, void *game_settings));   // the drop key
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));            // next value of a random walk
FN(unsigned char, walk_should_change, (void *state, Range r, double p));   // random yes/no
FN(float, random_rotation, (Range r));                                // random value in the range

enum { ITEM_MUSHROOM_STEW = 282, ITEM_RABBIT_STEW = 413 };

extern "C" void auto_soup_input_hook(InHook *hook) {
    if (hook->mod != auto_soup_module_descriptor)
        return;
    int stage = auto_soup_stage;
    if (stage == STAGE_CHECK_HEALTH || stage == STAGE_COOL_DOWN)
        return;
    hook->use = 1;
    auto_soup_input_consumed = 1;
}

extern "C" void auto_soup_enable(EvHook *hook) {
    if (hook->ev != auto_soup_module_descriptor || hook->on != 1)
        return;
    auto_soup_stage = STAGE_CHECK_HEALTH;
    auto_soup_busy = 0;
    auto_soup_step_delay = 0.0f;
    auto_soup_input_consumed = 0;
}

// A delay: `base` ticks plus a random 0-1 tick (now and then 0.5-2.5 more), scaled by "randomization".
extern "C" float auto_soup_asm_pins_keep(float base) {
    float slow = walk_update(auto_soup_rng_states, rot_make_floats(0.0f, 0.5f), 60.0);
    float extra = walk_update(auto_soup_rng_states, rot_make_floats(0.0f, 0.5f), 5.0) + slow;
    if (walk_should_change(auto_soup_random_walk_1, rot_make_floats(0.03f, 0.06f), 60.0))
        extra += random_rotation(rot_make_floats(0.5f, 2.5f));
    return extra * auto_soup_delay_scale + base;
}

// Wait a random delay of about `base` ticks.
static void add_delay(float base) {
    auto_soup_step_delay = auto_soup_asm_pins_keep(base) + auto_soup_step_delay;
}

// Find a soup in the hotbar (starting from the selected slot). Returns the slot, or -1.
static int find_soup(Ctx *ctx, void *inventory) {
    unsigned char skip_current = auto_soup_busy;   // second soup: the current slot holds the bowl
    int current = java_get_inventory_player_current_item(ctx->jvm, inventory);
    void *hotbar = java_get_inventory_player_main_inventory(ctx->jvm, inventory);
    int found = -1;
    for (int i = 0; i < 9; i++) {
        int slot = (current + i) % 9;
        if (skip_current & (slot == current))
            continue;
        void *jvm = ctx->jvm;
        JNIEnv *env = ((Jvm *)jvm)->env;
        void *stack = (*env)->GetObjectArrayElement(env, hotbar, slot);
        int item_id = stack ? java_call_static_item_get_id_from_item(jvm, java_call_item_stack_get_item(jvm, stack)) : 0;
        if (auto_soup_override_item == 1) {
            if (!id_vector_contains(auto_soup_custom_item_set, item_id))
                continue;
        } else if (item_id != ITEM_RABBIT_STEW && item_id != ITEM_MUSHROOM_STEW) {
            continue;
        }
        auto_soup_item_leaves_bowl = (item_id == ITEM_MUSHROOM_STEW) | (item_id == ITEM_RABBIT_STEW);
        found = slot;
        // Usually take the first one; now and then (1-8%) keep looking.
        if (!walk_should_change(auto_soup_random_walk_2, rot_make_floats(0.01f, 0.08f), 30.0))
            break;
    }
    return found;
}

extern "C" void auto_soup_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    // The player used the hotbar themselves while we were eating: switch off.
    if (auto_soup_input_consumed == 1 && (auto_soup_stage == STAGE_COOL_DOWN || auto_soup_stage == STAGE_CHECK_HEALTH)) {
        queue_module_toggle(ctx->module_manager, auto_soup_module_descriptor, 0);
        return;
    }
    if (ctx->in_game != 1) {
        auto_soup_busy = 0;
        auto_soup_stage = STAGE_CHECK_HEALTH;
        return;
    }

    // Up to 3 steps per tick, while there is no delay.
    for (int step = 0; step < 3; step++) {
        float delay = auto_soup_step_delay;
        if (delay > 0.0f) {
            auto_soup_step_delay = delay + -1.0f;
            return;
        }

        switch (auto_soup_stage) {
        case STAGE_CHECK_HEALTH: {
            float health = java_call_entity_living_base_get_health(ctx->jvm, ctx->player);
            if (health > auto_soup_single_soup_max_hp)
                return;
            add_delay(auto_soup_delays_refill_health);
            auto_soup_eat_another_one = auto_soup_double_soup_max_hp >= health;
            auto_soup_stage = STAGE_FIND_SOUP;
            break;
        }
        case STAGE_FIND_SOUP: {
            void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
            int found = find_soup(ctx, inventory);
            if (found == -1) {
                // No soup (left): switch back if we were eating the second one, else cool down.
                auto_soup_stage = STAGE_COOL_DOWN - auto_soup_busy;
                auto_soup_busy = 0;
                return;
            }
            auto_soup_soup_slot = found;
            int original;
            if (auto_soup_busy == 1) {
                original = auto_soup_original_slot;
            } else {
                original = java_get_inventory_player_current_item(ctx->jvm, inventory);
                auto_soup_original_slot = original;
                found = auto_soup_soup_slot;
            }
            auto_soup_busy = 0;
            // Already holding it: eat straight away, else switch first.
            auto_soup_stage = (found == original) | STAGE_SWITCH;
            break;
        }
        case STAGE_SWITCH:
            if (!claim_release_hotbar_module(auto_soup_module_descriptor)) {
                auto_soup_stage = STAGE_CHECK_HEALTH;
                return;
            }
            java_set_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player), auto_soup_soup_slot);
            auto_soup_stage = STAGE_EAT;
            add_delay(auto_soup_delays_refill_health_second);
            if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player)) {
                float x = auto_soup_step_delay;
                if (0.0f >= x)
                    auto_soup_step_delay = x + 1.0f;
            }
            break;
        case STAGE_EAT:
            java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));
            if (auto_soup_drop_bowls_value == 1 && auto_soup_item_leaves_bowl) {
                auto_soup_stage = STAGE_DROP_BOWL;
                float x = auto_soup_asm_pins_keep(auto_soup_delays_refill_health_second) + auto_soup_step_delay;
                auto_soup_step_delay = x > 0.0f ? x : x + 1.0f;
                break;
            }
            goto eaten;
        case STAGE_DROP_BOWL:
            java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_key_8_or_9(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));
        eaten:
            auto_soup_stage = STAGE_SWITCH_BACK;
            if (auto_soup_soup_slot != auto_soup_original_slot) {
                float x = auto_soup_asm_pins_keep(auto_soup_delays_refill_health_second) + auto_soup_step_delay;
                auto_soup_step_delay = x;
                if (0.0f >= x)
                    auto_soup_step_delay = x + 1.0f;
            }
            break;
        case STAGE_SWITCH_BACK:
            release_owner(auto_soup_module_descriptor);
            if (auto_soup_eat_another_one == 1) {
                auto_soup_eat_another_one = 0;
                auto_soup_busy = 1;
                auto_soup_stage = STAGE_FIND_SOUP;
                break;
            }
            java_set_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player), auto_soup_original_slot);
            auto_soup_stage = STAGE_COOL_DOWN;
            break;
        case STAGE_COOL_DOWN:
            auto_soup_busy = 0;
            add_delay(auto_soup_delays_refill_health_third);
            auto_soup_stage = STAGE_CHECK_HEALTH;
            break;
        }
    }
}
