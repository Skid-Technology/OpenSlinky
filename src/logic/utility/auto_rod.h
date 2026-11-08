// Auto Rod — Throws your fishing rod for you.
// category: utility   descriptor: 0x2f3520   settings block: 0x2f3718
// functions: gui auto_rod_gui, register auto_rod_register
// implementation: src/logic/utility/auto_rod.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_auto_rod); // "Main"
S(str_throwables); // "Throwables"
S(str_retract_speed_in_auto_rod); // "retract_speed"
S(str_delay_in_auto_rod); // "delay"
S(str_randomize_delays_in_auto_rod); // "randomize_delays"
S(str_always_switch_to_weapon); // "always_switch_to_weapon"
S(str_allow_throwables); // "allow_throwables"
S(str_throw_speed_in_auto_rod); // "throw_speed"
S(str_require_not_mouse_over); // "require_not_mouse_over"

D(auto_rod_require_not_mouse_over_setting); // setting bool "require_not_mouse_over"
D(auto_rod_allow_throwables_setting); // setting bool "allow_throwables"
D(auto_rod_always_switch_to_weapon_setting); // setting bool "always_switch_to_weapon"
D(auto_rod_delay_setting); // setting num "delay"
D(auto_rod_randomize_delays_setting); // setting bool "randomize_delays"
D(auto_rod_retract_speed_setting); // setting num "retract_speed"
D(auto_rod_throw_speed_setting); // setting num "throw_speed"

extern "C" void auto_rod_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_auto_rod);
    W_slider(window, gui, auto_rod_retract_speed_setting);
    W_slider(window, gui, auto_rod_delay_setting);
    W_bool(window, gui, auto_rod_randomize_delays_setting);
    W_bool(window, gui, auto_rod_always_switch_to_weapon_setting);
    W_label(gui, window, str_throwables);
    W_bool(window, gui, auto_rod_allow_throwables_setting);
    W_slider(window, gui, auto_rod_throw_speed_setting);
    W_label(gui, window, str_main_in_auto_rod);
    W_bool(window, gui, auto_rod_require_not_mouse_over_setting);
}

extern "C" void auto_rod_register(void *module) {
    R_num(module, str_retract_speed_in_auto_rod, auto_rod_retract_speed_setting);
    R_num(module, str_delay_in_auto_rod, auto_rod_delay_setting);
    R_bool(module, str_randomize_delays_in_auto_rod, auto_rod_randomize_delays_setting);
    R_bool(module, str_always_switch_to_weapon, auto_rod_always_switch_to_weapon_setting);
    R_bool(module, str_allow_throwables, auto_rod_allow_throwables_setting);
    R_num(module, str_throw_speed_in_auto_rod, auto_rod_throw_speed_setting);
    R_bool(module, str_require_not_mouse_over, auto_rod_require_not_mouse_over_setting);
}
