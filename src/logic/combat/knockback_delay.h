// Knockback Delay — Delays your knockback in certain scenarios.
// category: combat   descriptor: 0x2f40f8   settings block: 0x2f42f0
// functions: gui knockback_delay_gui, register knockback_delay_register, status knockback_delay_status
// implementation: src/logic/combat/knockback_delay.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_knockback_delay); // "Main"
S(str_conditions_in_knockback_delay); // "Conditions"
S(str_range_in_knockback_delay); // "range"
S(str_chance_in_knockback_delay); // "chance"
S(str_max_delay_in_knockback_delay); // "max_delay"
S(str_require_air); // "require_air"
S(str_require_target_in_knockback_delay); // "require_target"
S(str_require_mouse_in_knockback_delay); // "require_mouse"
S(str_require_weapon_in_knockback_delay); // "require_weapon"
S(str_0fms_in_knockback_delay); // "%.0fms"

D(knockback_delay_max_delay_setting);                    // setting num "max_delay"
extern "C" float knockback_delay_max_delay; // current value of setting "max_delay" (+0x30)
D(knockback_delay_require_air_setting);                    // setting bool "require_air"
D(knockback_delay_range_setting);                    // setting num "range"
D(knockback_delay_require_weapon_setting);                    // setting bool "require_weapon"
D(knockback_delay_require_mouse_setting);                    // setting bool "require_mouse"
D(knockback_delay_require_target_setting);                    // setting bool "require_target"
D(knockback_delay_chance_setting);                    // setting num "chance"

extern "C" void knockback_delay_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_knockback_delay);
    W_slider(window, gui, knockback_delay_range_setting);
    W_slider(window, gui, knockback_delay_chance_setting);
    W_slider(window, gui, knockback_delay_max_delay_setting);
    W_label(gui, window, str_conditions_in_knockback_delay);
    W_bool(window, gui, knockback_delay_require_air_setting);
    W_bool(window, gui, knockback_delay_require_target_setting);
    W_bool(window, gui, knockback_delay_require_mouse_setting);
    W_bool(window, gui, knockback_delay_require_weapon_setting);
}

extern "C" void knockback_delay_register(void *module) {
    R_num(module, str_range_in_knockback_delay, knockback_delay_range_setting);
    R_num(module, str_chance_in_knockback_delay, knockback_delay_chance_setting);
    R_num(module, str_max_delay_in_knockback_delay, knockback_delay_max_delay_setting);
    R_bool(module, str_require_air, knockback_delay_require_air_setting);
    R_bool(module, str_require_target_in_knockback_delay, knockback_delay_require_target_setting);
    R_bool(module, str_require_mouse_in_knockback_delay, knockback_delay_require_mouse_setting);
    R_bool(module, str_require_weapon_in_knockback_delay, knockback_delay_require_weapon_setting);
}

extern "C" void knockback_delay_status(void *self) {
    log_fmt(self, 0x20, str_0fms_in_knockback_delay, (double)knockback_delay_max_delay);

}
