// Weapons — Configure which items are considered weapons.
// category: combat   descriptor: 0x2ee930   settings block: 0x2eeb28
// functions: gui weapons_gui, register weapons_register
// implementation: src/logic/combat/weapons.cpp
#pragma once
#include "slinky/api.h"

S(str_item_types); // "Item types"
S(str_any_item_with); // "Any item with"
S(str_any_item_in); // "Any item in"
S(str_fist_in_weapons); // "fist"
S(str_swords_in_weapons); // "swords"
S(str_axes_in_weapons); // "axes"
S(str_extra_items); // "extra_items"
S(str_enchant_sharpness); // "enchant_sharpness"
S(str_enchant_knockback); // "enchant_knockback"
S(str_enchant_fire_aspect); // "enchant_fire_aspect"

D(weapons_fist_setting); // setting bool "fist"
D(weapons_bool_slot_1); // setting bool (slot)
D(weapons_bool_slot_2); // setting bool (slot)
D(weapons_bool_slot_3); // setting bool (slot)
D(weapons_bool_slot_4); // setting bool (slot)
D(weapons_bool_slot_5); // setting bool (slot)
D(weapons_bool_slot_6); // setting bool (slot)
D(weapons_bool_slot_7); // setting bool (slot)
D(weapons_bool_slot_8); // setting bool (slot)
D(weapons_bool_slot_9); // setting bool (slot)
D(weapons_enchant_sharpness_setting); // setting bool "enchant_sharpness"
D(weapons_enchant_knockback_setting); // setting bool "enchant_knockback"
D(weapons_enchant_fire_aspect_setting); // setting bool "enchant_fire_aspect"
D(weapons_axes_setting); // setting bool "axes"
D(weapons_swords_setting); // setting bool "swords"
D(weapons_extra_items_setting); // setting kind2 "extra_items"

extern "C" void weapons_gui(void *gui, void *window) {
    W_label(gui, window, str_item_types);
    W_bool(window, gui, weapons_fist_setting);
    W_bool(window, gui, weapons_swords_setting);
    W_bool(window, gui, weapons_axes_setting);
    W_items(window, gui, weapons_extra_items_setting);
    W_label(gui, window, str_any_item_with);
    W_bool(window, gui, weapons_enchant_sharpness_setting);
    W_bool(window, gui, weapons_enchant_knockback_setting);
    W_bool(window, gui, weapons_enchant_fire_aspect_setting);
    W_label(gui, window, str_any_item_in);
    W_bool(window, gui, weapons_bool_slot_1);
    W_bool(window, gui, weapons_bool_slot_2);
    W_bool(window, gui, weapons_bool_slot_3);
    W_bool(window, gui, weapons_bool_slot_4);
    W_bool(window, gui, weapons_bool_slot_5);
    W_bool(window, gui, weapons_bool_slot_6);
    W_bool(window, gui, weapons_bool_slot_7);
    W_bool(window, gui, weapons_bool_slot_8);
    W_bool(window, gui, weapons_bool_slot_9);
}

extern "C" void weapons_register(void *module) {
    char slot[2];
    R_bool(module, str_fist_in_weapons, weapons_fist_setting);
    R_bool(module, str_swords_in_weapons, weapons_swords_setting);
    R_bool(module, str_axes_in_weapons, weapons_axes_setting);
    R_kind2(module, str_extra_items, weapons_extra_items_setting);
    R_bool(module, str_enchant_sharpness, weapons_enchant_sharpness_setting);
    R_bool(module, str_enchant_knockback, weapons_enchant_knockback_setting);
    R_bool(module, str_enchant_fire_aspect, weapons_enchant_fire_aspect_setting);
    *(__int16 *)slot = 0x31;
    R_bool(module, slot, weapons_bool_slot_1);
    *(__int16 *)slot = 0x32;
    R_bool(module, slot, weapons_bool_slot_2);
    *(__int16 *)slot = 0x33;
    R_bool(module, slot, weapons_bool_slot_3);
    *(__int16 *)slot = 0x34;
    R_bool(module, slot, weapons_bool_slot_4);
    *(__int16 *)slot = 0x35;
    R_bool(module, slot, weapons_bool_slot_5);
    *(__int16 *)slot = 0x36;
    R_bool(module, slot, weapons_bool_slot_6);
    *(__int16 *)slot = 0x37;
    R_bool(module, slot, weapons_bool_slot_7);
    *(__int16 *)slot = 0x38;
    R_bool(module, slot, weapons_bool_slot_8);
    *(__int16 *)slot = 0x39;
    R_bool(module, slot, weapons_bool_slot_9);
}
