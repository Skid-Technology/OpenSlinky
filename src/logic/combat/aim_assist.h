// Aim Assist — Pulls your crosshair towards other players.
// category: combat   descriptor: 0x2e8cc0   settings block: 0x2e8eb8
// functions: gui aim_assist_gui, register aim_assist_register, status aim_assist_status
// implementation: src/logic/combat/aim_assist.cpp
#pragma once
#include "slinky/api.h"

S(str_aim); // "Aim"
S(str_multi_point); // "Multi-point"
S(str_target); // "Target"
S(str_conditions); // "Conditions"
S(str_mode_in_aim_assist); // "mode"
S(str_hor_speed); // "hor_speed"
S(str_ver_speed); // "ver_speed"
S(str_lock_on_vertical); // "lock_on_vertical"
S(str_silent_move_correct); // "silent_move_correct"
S(str_silent_ignore_manual_aim); // "silent_ignore_manual_aim"
S(str_predict_in_aim_assist); // "predict"
S(str_aim_randomization); // "aim_randomization"
S(str_hor_hitbox_correct); // "hor_hitbox_correct"
S(str_ver_hitbox_correct); // "ver_hitbox_correct"
S(str_mode_target); // "mode_target"
S(str_sort_by_in_aim_assist); // "sort_by"
S(str_allow_exceed_fov); // "allow_exceed_fov"
S(str_range_in_aim_assist); // "range"
S(str_fov_in_aim_assist); // "fov"
S(str_require_sprint); // "require_sprint"
S(str_require_mouse_pressed); // "require_mouse_pressed"
S(str_require_mouse_moved); // "require_mouse_moved"
S(str_disable_on_block_break); // "disable_on_block_break"
S(str_require_weapon); // "require_weapon"
S(str_ignore_invis); // "ignore_invis"
S(str_require_line_of_sight); // "require_line_of_sight"
S(str_s_in_aim_assist); // "%s"

D(aim_assist_fov_setting);                       // setting rng "fov"
D(aim_assist_hor_speed_setting);                       // setting num "hor_speed"
D(aim_assist_ver_speed_setting);                       // setting num "ver_speed"
D(aim_assist_lock_on_vertical_setting);                       // setting bool "lock_on_vertical"
D(aim_assist_require_mouse_moved_setting);                       // setting bool "require_mouse_moved"
D(aim_assist_enum_option_labels_1);                       // enum option labels (const char *[])
D(aim_assist_mode_setting);                       // setting enum "mode"
extern "C" unsigned aim_assist_mode; // current value of setting "mode" (+0x30)
D(aim_assist_silent_ignore_manual_aim_setting);                       // setting bool "silent_ignore_manual_aim"
D(aim_assist_silent_move_correct_setting);                       // setting bool "silent_move_correct"
D(aim_assist_range_setting);                       // setting rng "range"
D(aim_assist_require_sprint_setting);                       // setting bool "require_sprint"
D(aim_assist_require_mouse_pressed_setting);                       // setting bool "require_mouse_pressed"
D(aim_assist_disable_on_block_break_setting);                       // setting bool "disable_on_block_break"
D(aim_assist_require_weapon_setting);                       // setting bool "require_weapon"
D(aim_assist_mode_target_setting);                       // setting enum "mode_target"
extern "C" int aim_assist_mode_target;      // current value of setting "mode_target" (+0x30)
D(aim_assist_predict_setting);                       // setting num "predict"
D(aim_assist_hor_hitbox_correct_setting);                       // setting num "hor_hitbox_correct"
D(aim_assist_ver_hitbox_correct_setting);                       // setting num "ver_hitbox_correct"
D(aim_assist_aim_randomization_setting);                       // setting num "aim_randomization"
D(aim_assist_ignore_invis_setting);                       // setting bool "ignore_invis"
D(aim_assist_allow_exceed_fov_setting);                       // setting bool "allow_exceed_fov"
D(aim_assist_require_line_of_sight_setting);                       // setting bool "require_line_of_sight"
D(aim_assist_sort_by_setting);                       // setting enum "sort_by"

extern "C" void aim_assist_gui(void *gui, void *window) {
    W_label(gui, window, str_aim);
    W_enumw(window, gui, aim_assist_mode_setting);
    if (aim_assist_mode == 2) {
        W_bool(window, gui, aim_assist_lock_on_vertical_setting);
    } else if (aim_assist_mode == 3) {
        W_bool(window, gui, aim_assist_silent_move_correct_setting);
        W_bool(window, gui, aim_assist_silent_ignore_manual_aim_setting);
        W_slider(window, gui, aim_assist_hor_speed_setting);
        W_slider(window, gui, aim_assist_ver_speed_setting);
    } else {
        W_slider(window, gui, aim_assist_hor_speed_setting);
        W_slider(window, gui, aim_assist_ver_speed_setting);
    }
    W_slider(window, gui, aim_assist_predict_setting);
    W_slider(window, gui, aim_assist_aim_randomization_setting);
    W_label(gui, window, str_multi_point);
    W_slider(window, gui, aim_assist_hor_hitbox_correct_setting);
    W_slider(window, gui, aim_assist_ver_hitbox_correct_setting);
    W_label(gui, window, str_target);
    W_enumw(window, gui, aim_assist_mode_target_setting);
    if (aim_assist_mode_target == 0) {
        W_bool(window, gui, aim_assist_allow_exceed_fov_setting);
    } else if (aim_assist_mode_target == 1) {
        W_enumw(window, gui, aim_assist_sort_by_setting);
    }
    W_range(window, gui, aim_assist_range_setting);
    W_range(window, gui, aim_assist_fov_setting);
    W_label(gui, window, str_conditions);
    if (aim_assist_mode != 3)
        W_bool(window, gui, aim_assist_require_sprint_setting);
    W_bool(window, gui, aim_assist_require_mouse_pressed_setting);
    W_bool(window, gui, aim_assist_require_mouse_moved_setting);
    W_bool(window, gui, aim_assist_disable_on_block_break_setting);
    W_bool(window, gui, aim_assist_require_weapon_setting);
    W_bool(window, gui, aim_assist_ignore_invis_setting);
    W_bool(window, gui, aim_assist_require_line_of_sight_setting);
}

extern "C" void aim_assist_register(void *module) {
    R_enum(module, str_mode_in_aim_assist, aim_assist_mode_setting);
    R_num(module, str_hor_speed, aim_assist_hor_speed_setting);
    R_num(module, str_ver_speed, aim_assist_ver_speed_setting);
    R_bool(module, str_lock_on_vertical, aim_assist_lock_on_vertical_setting);
    R_bool(module, str_silent_move_correct, aim_assist_silent_move_correct_setting);
    R_bool(module, str_silent_ignore_manual_aim, aim_assist_silent_ignore_manual_aim_setting);
    R_num(module, str_predict_in_aim_assist, aim_assist_predict_setting);
    R_num(module, str_aim_randomization, aim_assist_aim_randomization_setting);
    R_num(module, str_hor_hitbox_correct, aim_assist_hor_hitbox_correct_setting);
    R_num(module, str_ver_hitbox_correct, aim_assist_ver_hitbox_correct_setting);
    R_enum(module, str_mode_target, aim_assist_mode_target_setting);
    R_enum(module, str_sort_by_in_aim_assist, aim_assist_sort_by_setting);
    R_bool(module, str_allow_exceed_fov, aim_assist_allow_exceed_fov_setting);
    R_rng(module, str_range_in_aim_assist, aim_assist_range_setting);
    R_rng(module, str_fov_in_aim_assist, aim_assist_fov_setting);
    R_bool(module, str_require_sprint, aim_assist_require_sprint_setting);
    R_bool(module, str_require_mouse_pressed, aim_assist_require_mouse_pressed_setting);
    R_bool(module, str_require_mouse_moved, aim_assist_require_mouse_moved_setting);
    R_bool(module, str_disable_on_block_break, aim_assist_disable_on_block_break_setting);
    R_bool(module, str_require_weapon, aim_assist_require_weapon_setting);
    R_bool(module, str_ignore_invis, aim_assist_ignore_invis_setting);
    R_bool(module, str_require_line_of_sight, aim_assist_require_line_of_sight_setting);
}

extern "C" void aim_assist_status(void *self) {
    log_fmt(self, 0x20, str_s_in_aim_assist, ((const char **)aim_assist_enum_option_labels_1)[aim_assist_mode]);

}
