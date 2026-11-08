// Fast Accel — Speeds up acceleration.
// category: move   descriptor: 0x2f27f8   settings block: 0x2f29f0
// functions: gui fast_accel_gui, register fast_accel_register
// implementation: src/logic/move/fast_accel.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_fast_accel); // "Main"
S(str_conditions_in_fast_accel); // "Conditions"
S(str_acceleration); // "acceleration"
S(str_require_not_damaged); // "require_not_damaged"
S(str_require_weapon_in_fast_accel); // "require_weapon"

D(fast_accel_acceleration_setting); // setting num "acceleration"
D(fast_accel_require_weapon_setting); // setting bool "require_weapon"
D(fast_accel_require_not_damaged_setting); // setting bool "require_not_damaged"

extern "C" void fast_accel_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_fast_accel);
    W_slider(window, gui, fast_accel_acceleration_setting);
    W_label(gui, window, str_conditions_in_fast_accel);
    W_bool(window, gui, fast_accel_require_not_damaged_setting);
    W_bool(window, gui, fast_accel_require_weapon_setting);
}

extern "C" void fast_accel_register(void *module) {
    R_num(module, str_acceleration, fast_accel_acceleration_setting);
    R_bool(module, str_require_not_damaged, fast_accel_require_not_damaged_setting);
    R_bool(module, str_require_weapon_in_fast_accel, fast_accel_require_weapon_setting);
}
