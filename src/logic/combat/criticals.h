// Criticals — Increases chance of landing critical hits.
// category: combat   descriptor: 0x2ec010   settings block: 0x2ec208
// functions: gui criticals_gui, register criticals_register, status criticals_status
// implementation: src/logic/combat/criticals.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_criticals); // "Main"
S(str_conditions_in_criticals); // "Conditions"
S(str_mode_in_criticals); // "mode"
S(str_chance_in_criticals); // "chance"
S(str_max_delay); // "max_delay"
S(str_timer_speed_in_criticals); // "timer_speed"
S(str_boost_in_criticals); // "boost"
S(str_require_weapon_in_criticals); // "require_weapon"
S(str_require_mouse_pressed_in_criticals); // "require_mouse_pressed"
S(str_s_in_criticals); // "%s"

D(criticals_max_delay_setting);                       // setting num "max_delay"
D(criticals_chance_setting);                       // setting num "chance"
D(criticals_require_mouse_pressed_setting);                       // setting bool "require_mouse_pressed"
D(criticals_require_weapon_setting);                       // setting bool "require_weapon"
D(criticals_enum_option_labels_1);                       // enum option labels (const char *[])
D(criticals_mode_setting);                       // setting enum "mode"
extern "C" unsigned criticals_mode; // current value of setting "mode" (+0x30)
D(criticals_timer_speed_setting);                       // setting num "timer_speed"
D(criticals_boost_setting);                       // setting num "boost"

extern "C" void criticals_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_criticals);
    W_enumw(window, gui, criticals_mode_setting);
    W_slider(window, gui, criticals_chance_setting);
    if (criticals_mode != 0) {
        W_slider(window, gui, criticals_timer_speed_setting);
        W_slider(window, gui, criticals_boost_setting);
    } else {
        W_slider(window, gui, criticals_max_delay_setting);
    }
    W_label(gui, window, str_conditions_in_criticals);
    W_bool(window, gui, criticals_require_weapon_setting);
    W_bool(window, gui, criticals_require_mouse_pressed_setting);
}

extern "C" void criticals_register(void *module) {
    R_enum(module, str_mode_in_criticals, criticals_mode_setting);
    R_num(module, str_chance_in_criticals, criticals_chance_setting);
    R_num(module, str_max_delay, criticals_max_delay_setting);
    R_num(module, str_timer_speed_in_criticals, criticals_timer_speed_setting);
    R_num(module, str_boost_in_criticals, criticals_boost_setting);
    R_bool(module, str_require_weapon_in_criticals, criticals_require_weapon_setting);
    R_bool(module, str_require_mouse_pressed_in_criticals, criticals_require_mouse_pressed_setting);
}

extern "C" void criticals_status(void *self) {
    log_fmt(self, 0x20, str_s_in_criticals, ((const char **)criticals_enum_option_labels_1)[criticals_mode]);

}
