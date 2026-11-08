// Auto Weapon — Selects your weapon when aiming on players.
// category: combat   descriptor: 0x2f5af8   settings block: 0x2f5cf0
// functions: gui auto_weapon_gui, register auto_weapon_register
// implementation: src/logic/combat/auto_weapon.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_auto_weapon); // "Main"
S(str_conditions_in_auto_weapon); // "Conditions"
S(str_activation_time_in_auto_weapon); // "activation_time"
S(str_require_mouse_pressed_in_auto_weapon); // "require_mouse_pressed"

D(auto_weapon_activation_time_setting); // setting num "activation_time"
D(auto_weapon_require_mouse_pressed_setting); // setting bool "require_mouse_pressed"

extern "C" void auto_weapon_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_auto_weapon);
    W_slider(window, gui, auto_weapon_activation_time_setting);
    W_label(gui, window, str_conditions_in_auto_weapon);
    W_bool(window, gui, auto_weapon_require_mouse_pressed_setting);
}

extern "C" void auto_weapon_register(void *module) {
    R_num(module, str_activation_time_in_auto_weapon, auto_weapon_activation_time_setting);
    R_bool(module, str_require_mouse_pressed_in_auto_weapon, auto_weapon_require_mouse_pressed_setting);
}
