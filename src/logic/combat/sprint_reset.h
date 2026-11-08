// Sprint Reset — Resets your sprint state to deal more knockback.
// category: combat   descriptor: 0x2efd00   settings block: 0x2efef8
// functions: gui sprint_reset_gui, register sprint_reset_register, status sprint_reset_status
// implementation: src/logic/combat/sprint_reset.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_sprint_reset); // "Main"
S(str_conditions_in_sprint_reset); // "Conditions"
S(str_mode_in_sprint_reset); // "mode"
S(str_delay_in_sprint_reset); // "delay"
S(str_duration_in_sprint_reset); // "duration"
S(str_randomize_in_sprint_reset); // "randomize"
S(str_wait_for_damage); // "wait_for_damage"
S(str_require_weapon_in_sprint_reset); // "require_weapon"
S(str_s_in_sprint_reset); // "%s"

D(sprint_reset_enum_option_labels_1);                       // enum option labels (const char *[])
D(sprint_reset_mode_setting);                       // setting enum "mode"
extern "C" unsigned sprint_reset_mode; // current value of setting "mode" (+0x30)
D(sprint_reset_delay_setting);                       // setting num "delay"
D(sprint_reset_randomize_setting);                       // setting bool "randomize"
D(sprint_reset_wait_for_damage_setting);                       // setting bool "wait_for_damage"
D(sprint_reset_require_weapon_setting);                       // setting bool "require_weapon"
D(sprint_reset_duration_setting);                       // setting num "duration"

extern "C" void sprint_reset_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_sprint_reset);
    W_enumw(window, gui, sprint_reset_mode_setting);
    W_slider(window, gui, sprint_reset_delay_setting);
    W_slider(window, gui, sprint_reset_duration_setting);
    W_bool(window, gui, sprint_reset_randomize_setting);
    W_label(gui, window, str_conditions_in_sprint_reset);
    W_bool(window, gui, sprint_reset_wait_for_damage_setting);
    W_bool(window, gui, sprint_reset_require_weapon_setting);
}

extern "C" void sprint_reset_register(void *module) {
    R_enum(module, str_mode_in_sprint_reset, sprint_reset_mode_setting);
    R_num(module, str_delay_in_sprint_reset, sprint_reset_delay_setting);
    R_num(module, str_duration_in_sprint_reset, sprint_reset_duration_setting);
    R_bool(module, str_randomize_in_sprint_reset, sprint_reset_randomize_setting);
    R_bool(module, str_wait_for_damage, sprint_reset_wait_for_damage_setting);
    R_bool(module, str_require_weapon_in_sprint_reset, sprint_reset_require_weapon_setting);
}

extern "C" void sprint_reset_status(void *self) {
    log_fmt(self, 0x20, str_s_in_sprint_reset, ((const char **)sprint_reset_enum_option_labels_1)[sprint_reset_mode]);

}
