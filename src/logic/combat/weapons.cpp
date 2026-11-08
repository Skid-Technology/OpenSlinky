// Weapons - decides which items count as a "weapon" for the other combat modules.
//
// Modules with "require_weapon" (and Auto Weapon) ask here. An item is a weapon when:
//   - its hotbar slot is ticked in the settings, or
//   - it has an enchantment that is ticked (sharpness, knockback, fire aspect), or
//   - it is a sword / axe and that class is ticked, or
//   - its id is in the user's custom item list.
// An empty hand counts when "fist" is ticked.
//
// Functions:
//   weapons_check_held_item_cached    is the held item a weapon? (cached for one tick)
//   weapons_is_item_allowed_by_class  sword / axe / custom list check for one item
//   weapons_find_first_weapon_slot    first hotbar slot with a weapon, or -1
//   weapons_on_shutdown               free the custom item list
#include "slinky/engine.h"

// A per-slot checkbox setting (32 bytes each); the value is at +0x18.
struct SlotSetting { char padding_00[0x18]; unsigned char on; char padding_19[7]; };
// A list of item ids (start / end pointers).
struct IdCache { void *p, *q; };

// ---- settings ----
extern "C" unsigned char weapons_fist_value;                  // "fist"
extern "C" SlotSetting weapons_bool_slot_1[];                 // "slot 1" .. "slot 9"
extern "C" unsigned char weapons_enchant_sharpness_value;     // "enchant_sharpness"
extern "C" unsigned char weapons_enchant_knockback_value;     // "enchant_knockback"
extern "C" unsigned char weapons_enchant_fire_aspect_value;   // "enchant_fire_aspect"
extern "C" unsigned char weapons_axes_value;                  // "axes"
extern "C" unsigned char weapons_swords_value;                // "swords"
extern "C" IdCache weapons_slot_id_vector;                    // custom item ids

// ---- cache: the answer for the current tick ----
extern "C" int weapons_cached_key;                // tick the answer is for
extern "C" unsigned char weapons_cached_verdict;  // the answer
extern "C" unsigned weapons_cached_enchant_flags; // enchantments of the held item

// ---- game / engine functions ----
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(unsigned, get_weapon_enchant_flags, (void *jvm, void *item));                 // 1 = sharpness, 2 = knockback, 4+ = fire aspect
FN(void *, java_call_item_stack_get_item, (void *jvm, void *item_stack));
FN(unsigned char, weapons_is_item_allowed_by_class_entry, (void *jvm, void *item));   // = weapons_is_item_allowed_by_class
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned char, id_vector_contains, (IdCache *cache, int id));
FN(void, libc_free, (void *ptr));

// Is an enchantment on the item one of the ticked ones?
static bool has_allowed_enchant(unsigned flags) {
    if ((flags & 1) && weapons_enchant_sharpness_value)
        return true;
    if ((flags & 2) && weapons_enchant_knockback_value)
        return true;
    if (flags >= 4 && weapons_enchant_fire_aspect_value)
        return true;
    return false;
}

extern "C" unsigned char weapons_check_held_item_cached(void *jvm, void *entity_player, unsigned *out) {
    int tick = java_get_entity_ticks_existed(jvm, entity_player);
    if (tick != weapons_cached_key) {
        // New tick: work the answer out again.
        weapons_cached_key = tick;
        weapons_cached_enchant_flags = 0;
        void *inventory = java_get_entity_player_inventory(jvm, entity_player);
        int slot = java_get_inventory_player_current_item(jvm, inventory);
        void *stack = java_call_iinventory_get_stack_in_slot(jvm, inventory, slot);
        unsigned char is_weapon;
        if (!stack) {
            is_weapon = weapons_fist_value;
        } else {
            unsigned enchants = get_weapon_enchant_flags(jvm, stack);
            weapons_cached_enchant_flags = enchants;
            if (weapons_bool_slot_1[slot].on || has_allowed_enchant(enchants))
                is_weapon = 1;
            else
                is_weapon = weapons_is_item_allowed_by_class_entry(jvm, java_call_item_stack_get_item(jvm, stack));
        }
        weapons_cached_verdict = is_weapon;
    }
    if (out)
        *out = weapons_cached_enchant_flags;
    return weapons_cached_verdict;
}

extern "C" unsigned char weapons_is_item_allowed_by_class(void *jvm, void *item) {
    if (item) {
        int item_id = java_call_static_item_get_id_from_item(jvm, item);
        switch (item_id) {
        case 267: case 268: case 272: case 276: case 283:   // swords (iron, wood, stone, diamond, gold)
            return weapons_swords_value;
        case 258: case 271: case 275: case 279: case 286:   // axes (iron, wood, stone, diamond, gold)
            return weapons_axes_value;
        default:
            if (item_id)
                return id_vector_contains(&weapons_slot_id_vector, item_id);
        }
    }
    return weapons_fist_value;
}

// ctx points at the Ctx (its first field is the JVM handle).
extern "C" int weapons_find_first_weapon_slot(void **ctx, void *player) {
    for (long slot = 0; slot < 9; slot++) {
        if (weapons_bool_slot_1[slot].on)
            return (int)slot;
        void *stack = java_call_iinventory_get_stack_in_slot(*ctx, player, (int)slot);
        if (!stack)
            continue;
        void *jvm = *ctx;
        if (has_allowed_enchant(get_weapon_enchant_flags(jvm, stack)))
            return (int)slot;
        if (weapons_is_item_allowed_by_class_entry(jvm, java_call_item_stack_get_item(jvm, stack)))
            return (int)slot;
    }
    return -1;
}

extern "C" void weapons_on_shutdown(Hook *hook) {
    libc_free(weapons_slot_id_vector.p);
    weapons_slot_id_vector = IdCache{};
}
