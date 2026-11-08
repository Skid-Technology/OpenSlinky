// Timer — Modifies the speed the game runs at.
// category: utility   descriptor: 0x2eba10   settings block: 0x2ebc08
// functions: gui timer_gui, register timer_register, status timer_status
// implementation: src/logic/utility/timer.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_timer); // "Main"
S(str_conditions_in_timer); // "Conditions"
S(str_speed_in_timer); // "speed"
S(str_require_mouse_in_timer); // "require_mouse"
S(str_require_damage_in_timer); // "require_damage"
S(str_require_falling); // "require_falling"
S(fmt_float_2dp_x_b); // "%.2fx"

D(timer_speed_setting);                    // setting num "speed"
extern "C" float timer_speed_value; // current value of setting "speed" (+0x30)
D(timer_require_mouse_setting);                    // setting bool "require_mouse"
D(timer_require_falling_setting);                    // setting bool "require_falling"
D(timer_require_damage_setting);                    // setting bool "require_damage"

extern "C" void timer_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_timer);
    W_slider(window, gui, timer_speed_setting);
    W_label(gui, window, str_conditions_in_timer);
    W_bool(window, gui, timer_require_mouse_setting);
    W_bool(window, gui, timer_require_damage_setting);
    W_bool(window, gui, timer_require_falling_setting);
}

extern "C" void timer_register(void *module) {
    R_num(module, str_speed_in_timer, timer_speed_setting);
    R_bool(module, str_require_mouse_in_timer, timer_require_mouse_setting);
    R_bool(module, str_require_damage_in_timer, timer_require_damage_setting);
    R_bool(module, str_require_falling, timer_require_falling_setting);
}

extern "C" void timer_status(void *self) {
    log_fmt(self, 0x20, fmt_float_2dp_x_b, (double)timer_speed_value);

}
