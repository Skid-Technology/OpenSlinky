// Auto Block — Blocks your sword for you in combat.
// category: combat   descriptor: 0x2eaff8   settings block: 0x2eb1f0
// functions: gui auto_block_gui, register auto_block_register
// implementation: src/logic/combat/auto_block.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_auto_block); // "Main"
S(str_force_block_animation); // "Force block animation"
S(str_lag); // "Lag"
S(str_conditions_in_auto_block); // "Conditions"
S(str_range_in_auto_block); // "range"
S(str_max_hurt_time); // "max_hurt_time"
S(str_max_hold_time); // "max_hold_time"
S(str_fake_block_animation); // "fake_block_animation"
S(str_fake_block_animation_require_t); // "fake_block_animation_require_target"
S(str_lag_chance); // "lag_chance"
S(str_lag_duration); // "lag_duration"
S(str_lag_flush_attack); // "lag_flush_attack"
S(str_lag_reblock); // "lag_reblock"
S(str_require_left_mouse_pressed); // "require_left_mouse_pressed"
S(str_require_right_mouse_pressed); // "require_right_mouse_pressed"
S(str_require_damage); // "require_damage"

D(auto_block_max_hold_time_setting); // setting num "max_hold_time"
D(auto_block_lag_chance_setting); // setting num "lag_chance"
D(auto_block_lag_duration_setting); // setting num "lag_duration"
D(auto_block_max_hurt_time_setting); // setting num "max_hurt_time"
D(auto_block_require_left_mouse_pressed_setting); // setting bool "require_left_mouse_pressed"
D(auto_block_require_right_mouse_pressed_setting); // setting bool "require_right_mouse_pressed"
D(auto_block_require_damage_setting); // setting bool "require_damage"
D(auto_block_fake_block_animation_setting); // setting bool "fake_block_animation"
D(auto_block_fake_block_animation); // current value of setting "fake_block_animation" (+0x18)
D(auto_block_fake_block_animation_require_target_setting); // setting bool "fake_block_animation_require_target"
D(auto_block_range_setting); // setting num "range"
D(auto_block_lag_flush_attack_setting); // setting bool "lag_flush_attack"
D(auto_block_lag_reblock_setting); // setting bool "lag_reblock"

extern "C" void auto_block_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_auto_block);
    W_slider(window, gui, auto_block_range_setting);
    W_slider(window, gui, auto_block_max_hurt_time_setting);
    W_slider(window, gui, auto_block_max_hold_time_setting);
    W_label(gui, window, str_force_block_animation);
    W_bool(window, gui, auto_block_fake_block_animation_setting);
    if (*(unsigned char *)auto_block_fake_block_animation == 1) {
        W_bool(window, gui, auto_block_fake_block_animation_require_target_setting);
    }
    W_label(gui, window, str_lag);
    W_slider(window, gui, auto_block_lag_chance_setting);
    W_slider(window, gui, auto_block_lag_duration_setting);
    W_bool(window, gui, auto_block_lag_flush_attack_setting);
    W_bool(window, gui, auto_block_lag_reblock_setting);
    W_label(gui, window, str_conditions_in_auto_block);
    W_bool(window, gui, auto_block_require_left_mouse_pressed_setting);
    W_bool(window, gui, auto_block_require_right_mouse_pressed_setting);
    W_bool(window, gui, auto_block_require_damage_setting);
}

extern "C" void auto_block_register(void *module) {
    R_num(module, str_range_in_auto_block, auto_block_range_setting);
    R_num(module, str_max_hurt_time, auto_block_max_hurt_time_setting);
    R_num(module, str_max_hold_time, auto_block_max_hold_time_setting);
    R_bool(module, str_fake_block_animation, auto_block_fake_block_animation_setting);
    R_bool(module, str_fake_block_animation_require_t, auto_block_fake_block_animation_require_target_setting);
    R_num(module, str_lag_chance, auto_block_lag_chance_setting);
    R_num(module, str_lag_duration, auto_block_lag_duration_setting);
    R_bool(module, str_lag_flush_attack, auto_block_lag_flush_attack_setting);
    R_bool(module, str_lag_reblock, auto_block_lag_reblock_setting);
    R_bool(module, str_require_left_mouse_pressed, auto_block_require_left_mouse_pressed_setting);
    R_bool(module, str_require_right_mouse_pressed, auto_block_require_right_mouse_pressed_setting);
    R_bool(module, str_require_damage, auto_block_require_damage_setting);
}
