// Keep Sprint — Keeps your sprint speed when attacking players.
// category: move   descriptor: 0x2ef700   settings block: 0x2ef8f8
// functions: gui keep_sprint_gui, register keep_sprint_register, status keep_sprint_status
// implementation: src/logic/move/keep_sprint.cpp
#pragma once
#include "slinky/api.h"

extern "C" float factor_100_in_keep_sprint; // 100
S(str_0f_in_keep_sprint);                     // "%.0f%%"
S(str_main_in_keep_sprint);                     // "Main"
S(str_conditions_in_keep_sprint);                     // "Conditions"
S(str_chance_in_keep_sprint);                     // "chance"
S(str_motion_mult);                     // "motion_mult"
S(str_allow_reduce);                     // "allow_reduce"
S(str_require_on_ground);                     // "require_on_ground"

D(keep_sprint_chance_setting);                    // setting num "chance"
D(keep_sprint_motion_mult_setting);                    // setting num "motion_mult"
extern "C" float keep_sprint_motion_mult; // current value of setting "motion_mult" (+0x30)
D(keep_sprint_require_on_ground_setting);                    // setting bool "require_on_ground"
D(keep_sprint_allow_reduce_setting);                    // setting bool "allow_reduce"

extern "C" void keep_sprint_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_keep_sprint);
    W_slider(window, gui, keep_sprint_chance_setting);
    W_slider(window, gui, keep_sprint_motion_mult_setting);
    W_bool(window, gui, keep_sprint_allow_reduce_setting);
    W_label(gui, window, str_conditions_in_keep_sprint);
    W_bool(window, gui, keep_sprint_require_on_ground_setting);
}

extern "C" void keep_sprint_register(void *module) {
    R_num(module, str_chance_in_keep_sprint, keep_sprint_chance_setting);
    R_num(module, str_motion_mult, keep_sprint_motion_mult_setting);
    R_bool(module, str_allow_reduce, keep_sprint_allow_reduce_setting);
    R_bool(module, str_require_on_ground, keep_sprint_require_on_ground_setting);
}

extern "C" void keep_sprint_status(void *self) {
    log_fmt(self, 0x20, str_0f_in_keep_sprint, (double)(keep_sprint_motion_mult * factor_100_in_keep_sprint));

}
