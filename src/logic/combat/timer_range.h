// Timer Range — Abuses timer lag to teleport you.
// category: combat   descriptor: 0x2f3160   settings block: 0x2f3358
// functions: gui timer_range_gui, register timer_range_register, status timer_range_status
// implementation: src/logic/combat/timer_range.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_timer_range); // "Main"
S(str_conditions_in_timer_range); // "Conditions"
S(str_mode_in_timer_range); // "mode"
S(str_min_dist); // "min_dist"
S(str_duration_in_timer_range); // "duration"
S(str_duration_legit); // "duration_legit"
S(str_cooldown_in_timer_range); // "cooldown"
S(str_block_while_frozen_in_timer_range); // "block_while_frozen"
S(str_require_mouse_pressed_in_timer_range); // "require_mouse_pressed"
S(str_require_weapon_in_timer_range); // "require_weapon"
S(str_require_not_using_item_in_timer_range); // "require_not_using_item"
S(str_s_in_timer_range); // "%s"

D(timer_range_require_mouse_pressed_setting);                       // setting bool "require_mouse_pressed"
D(timer_range_require_weapon_setting);                       // setting bool "require_weapon"
D(timer_range_require_not_using_item_setting);                       // setting bool "require_not_using_item"
D(timer_range_enum_option_labels_1);                       // enum option labels (const char *[])
D(timer_range_mode_setting);                       // setting enum "mode"
extern "C" unsigned timer_range_mode; // current value of setting "mode" (+0x30)
D(timer_range_block_while_frozen_setting);                       // setting bool "block_while_frozen"
D(timer_range_cooldown_setting);                       // setting num "cooldown"
D(timer_range_duration_setting);                       // setting num "duration"
D(timer_range_duration_legit_setting);                       // setting num "duration_legit"
D(timer_range_min_dist_setting);                       // setting num "min_dist"

extern "C" void timer_range_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_timer_range);
    W_enumw(window, gui, timer_range_mode_setting);
    W_slider(window, gui, timer_range_min_dist_setting);
    W_slider(window, gui, timer_range_mode == 0 ? timer_range_duration_setting : timer_range_duration_legit_setting);
    W_slider(window, gui, timer_range_cooldown_setting);
    if (timer_range_mode == 0)
        W_bool(window, gui, timer_range_block_while_frozen_setting);
    W_label(gui, window, str_conditions_in_timer_range);
    W_bool(window, gui, timer_range_require_mouse_pressed_setting);
    W_bool(window, gui, timer_range_require_weapon_setting);
    W_bool(window, gui, timer_range_require_not_using_item_setting);
}

extern "C" void timer_range_register(void *module) {
    R_enum(module, str_mode_in_timer_range, timer_range_mode_setting);
    R_num(module, str_min_dist, timer_range_min_dist_setting);
    R_num(module, str_duration_in_timer_range, timer_range_duration_setting);
    R_num(module, str_duration_legit, timer_range_duration_legit_setting);
    R_num(module, str_cooldown_in_timer_range, timer_range_cooldown_setting);
    R_bool(module, str_block_while_frozen_in_timer_range, timer_range_block_while_frozen_setting);
    R_bool(module, str_require_mouse_pressed_in_timer_range, timer_range_require_mouse_pressed_setting);
    R_bool(module, str_require_weapon_in_timer_range, timer_range_require_weapon_setting);
    R_bool(module, str_require_not_using_item_in_timer_range, timer_range_require_not_using_item_setting);
}

extern "C" void timer_range_status(void *self) {
    log_fmt(self, 0x20, str_s_in_timer_range, ((const char **)timer_range_enum_option_labels_1)[timer_range_mode]);

}
