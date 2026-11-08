// Reach — Increases your melee range.
// category: combat   descriptor: 0x2ea550   settings block: 0x2ea748
// functions: gui reach_gui, register reach_register, status reach_status
// implementation: src/logic/combat/reach.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_reach); // "Main"
S(str_advanced); // "Advanced"
S(str_conditions_in_reach); // "Conditions"
S(str_distance_in_reach); // "distance"
S(str_activate_time); // "activate_time"
S(str_require_sprint_in_reach); // "require_sprint"
S(fmt_float_2dp_b); // "%.2f"

D(reach_distance_setting);                    // setting num "distance"
extern "C" float reach_distance; // current value of setting "distance" (+0x30)
D(reach_activate_time_setting);                    // setting num "activate_time"
D(reach_require_sprint_setting);                    // setting bool "require_sprint"

extern "C" void reach_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_reach);
    W_slider(window, gui, reach_distance_setting);
    W_label(gui, window, str_advanced);
    W_slider(window, gui, reach_activate_time_setting);
    W_label(gui, window, str_conditions_in_reach);
    W_bool(window, gui, reach_require_sprint_setting);
}

extern "C" void reach_register(void *module) {
    R_num(module, str_distance_in_reach, reach_distance_setting);
    R_num(module, str_activate_time, reach_activate_time_setting);
    R_bool(module, str_require_sprint_in_reach, reach_require_sprint_setting);
}

extern "C" void reach_status(void *self) {
    log_fmt(self, 0x20, fmt_float_2dp_b, (double)reach_distance);

}
