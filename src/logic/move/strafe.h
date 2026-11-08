// Strafe — Allows faster strafing.
// category: move   descriptor: 0x2f24e8   settings block: 0x2f26e0
// functions: gui strafe_gui, register strafe_register
// implementation: src/logic/move/strafe.cpp
#pragma once
#include "slinky/api.h"

S(str_strength); // "Strength"
S(str_conditions_in_strafe); // "Conditions"
S(str_strength_jump); // "strength_jump"
S(str_strength_ground); // "strength_ground"
S(str_strength_air); // "strength_air"
S(str_max_hurt_time_in_strafe); // "max_hurt_time"
S(str_require_weapon_in_strafe); // "require_weapon"

D(strafe_max_hurt_time_setting); // setting num "max_hurt_time"
D(strafe_strength_air_setting); // setting num "strength_air"
D(strafe_strength_jump_setting); // setting num "strength_jump"
D(strafe_strength_ground_setting); // setting num "strength_ground"
D(strafe_require_weapon_setting); // setting bool "require_weapon"

extern "C" void strafe_gui(void *gui, void *window) {
    W_label(gui, window, str_strength);
    W_slider(window, gui, strafe_strength_jump_setting);
    W_slider(window, gui, strafe_strength_ground_setting);
    W_slider(window, gui, strafe_strength_air_setting);
    W_label(gui, window, str_conditions_in_strafe);
    W_slider(window, gui, strafe_max_hurt_time_setting);
    W_bool(window, gui, strafe_require_weapon_setting);
}

extern "C" void strafe_register(void *module) {
    R_num(module, str_strength_jump, strafe_strength_jump_setting);
    R_num(module, str_strength_ground, strafe_strength_ground_setting);
    R_num(module, str_strength_air, strafe_strength_air_setting);
    R_num(module, str_max_hurt_time_in_strafe, strafe_max_hurt_time_setting);
    R_bool(module, str_require_weapon_in_strafe, strafe_require_weapon_setting);
}
