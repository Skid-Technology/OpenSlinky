// Inv Manager — Organises your inventory for you.
// category: utility   descriptor: 0x2f4c40   settings block: 0x2f4e38
// functions: gui inv_manager_gui, register inv_manager_register
// implementation: src/logic/utility/inv_manager.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_inv_manager); // "Main"
S(str_equip_armor); // "Equip armor"
S(str_sort_hotbar); // "Sort hotbar"
S(str_delay_after_open_in_inv_manager); // "delay_after_open"
S(str_speed_in_inv_manager); // "speed"
S(str_smart_speed_in_inv_manager); // "smart_speed"
S(str_randomize_in_inv_manager); // "randomize"
S(str_equip_armor_in_inv_manager); // "equip_armor"
S(str_sort_hotbar_in_inv_manager); // "sort_hotbar"
S(str_sort_hotbar_fallback); // "sort_hotbar_fallback"

D(inv_manager_delay_after_open_setting); // setting num "delay_after_open"
D(inv_manager_equip_armor_setting); // setting bool "equip_armor"
D(inv_manager_sort_hotbar_setting); // setting bool "sort_hotbar"
D(inv_manager_sort_hotbar); // current value of setting "sort_hotbar" (+0x18)
D(inv_manager_enum_slot_1); // setting enum (slot)
D(inv_manager_enum_slot_2); // setting enum (slot)
D(inv_manager_enum_slot_3); // setting enum (slot)
D(inv_manager_enum_slot_4); // setting enum (slot)
D(inv_manager_enum_slot_5); // setting enum (slot)
D(inv_manager_enum_slot_6); // setting enum (slot)
D(inv_manager_enum_slot_7); // setting enum (slot)
D(inv_manager_enum_slot_8); // setting enum (slot)
D(inv_manager_enum_slot_9); // setting enum (slot)
D(inv_manager_sort_hotbar_fallback_setting); // setting bool "sort_hotbar_fallback"
D(inv_manager_randomize_setting); // setting bool "randomize"
D(inv_manager_speed_setting); // setting num "speed"
D(inv_manager_smart_speed_setting); // setting bool "smart_speed"

extern "C" void inv_manager_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_inv_manager);
    W_slider(window, gui, inv_manager_delay_after_open_setting);
    W_slider(window, gui, inv_manager_speed_setting);
    W_bool(window, gui, inv_manager_smart_speed_setting);
    W_bool(window, gui, inv_manager_randomize_setting);
    W_label(gui, window, str_equip_armor);
    W_bool(window, gui, inv_manager_equip_armor_setting);
    W_label(gui, window, str_sort_hotbar);
    W_bool(window, gui, inv_manager_sort_hotbar_setting);
    if (*(unsigned char *)inv_manager_sort_hotbar == 1) {
        W_bool(window, gui, inv_manager_sort_hotbar_fallback_setting);
        W_enumw(window, gui, inv_manager_enum_slot_1);
        W_enumw(window, gui, inv_manager_enum_slot_2);
        W_enumw(window, gui, inv_manager_enum_slot_3);
        W_enumw(window, gui, inv_manager_enum_slot_4);
        W_enumw(window, gui, inv_manager_enum_slot_5);
        W_enumw(window, gui, inv_manager_enum_slot_6);
        W_enumw(window, gui, inv_manager_enum_slot_7);
        W_enumw(window, gui, inv_manager_enum_slot_8);
        W_enumw(window, gui, inv_manager_enum_slot_9);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void inv_manager_register(void *module) {
    char slot[2];
    R_num(module, str_delay_after_open_in_inv_manager, inv_manager_delay_after_open_setting);
    R_num(module, str_speed_in_inv_manager, inv_manager_speed_setting);
    R_bool(module, str_smart_speed_in_inv_manager, inv_manager_smart_speed_setting);
    R_bool(module, str_randomize_in_inv_manager, inv_manager_randomize_setting);
    R_bool(module, str_equip_armor_in_inv_manager, inv_manager_equip_armor_setting);
    R_bool(module, str_sort_hotbar_in_inv_manager, inv_manager_sort_hotbar_setting);
    R_bool(module, str_sort_hotbar_fallback, inv_manager_sort_hotbar_fallback_setting);
    *(__int16 *)slot = 0x31;
    R_enum(module, slot, inv_manager_enum_slot_1);
    *(__int16 *)slot = 0x32;
    R_enum(module, slot, inv_manager_enum_slot_2);
    *(__int16 *)slot = 0x33;
    R_enum(module, slot, inv_manager_enum_slot_3);
    *(__int16 *)slot = 0x34;
    R_enum(module, slot, inv_manager_enum_slot_4);
    *(__int16 *)slot = 0x35;
    R_enum(module, slot, inv_manager_enum_slot_5);
    *(__int16 *)slot = 0x36;
    R_enum(module, slot, inv_manager_enum_slot_6);
    *(__int16 *)slot = 0x37;
    R_enum(module, slot, inv_manager_enum_slot_7);
    *(__int16 *)slot = 0x38;
    R_enum(module, slot, inv_manager_enum_slot_8);
    *(__int16 *)slot = 0x39;
    R_enum(module, slot, inv_manager_enum_slot_9);
}
