// Velocity — Reduces the amount of knockback you take.
// category: combat   descriptor: 0x2ea7f0   settings block: 0x2ea9e8
// functions: gui velocity_gui, register velocity_register, status velocity_status
// implementation: src/logic/combat/velocity.cpp
#pragma once
#include "slinky/api.h"

extern "C" float factor_100; // 100
S(str_0f_in_velocity);                     // "%.0f%%"
S(str_main_in_velocity);                     // "Main"
S(str_conditions_in_velocity);                     // "Conditions"
S(str_mode_in_velocity);                     // "mode"
S(str_reduce_chance);                     // "reduce_chance"
S(str_fov_in_velocity);                     // "fov"
S(str_reduce_amount);                     // "reduce_amount"
S(str_reduce_delay_in_velocity);                     // "reduce_delay"
S(str_jump_chance);                     // "jump_chance"
S(str_reduce_multiple_per_tick);                     // "reduce_multiple_per_tick"
S(str_ignore_vertical_in_velocity);                     // "ignore_vertical"
S(str_ignore_chance);                     // "ignore_chance"
S(str_require_target);                     // "require_target"
S(str_require_mouse);                     // "require_mouse"
S(str_require_forward);                     // "require_forward"
S(str_require_weapon_in_velocity);                     // "require_weapon"
S(str_require_sprinting_jump);                     // "require_sprinting_jump"
S(str_s_in_velocity);                     // "%s"

D(velocity_enum_option_labels_1);                       // enum option labels (const char *[])
D(velocity_mode_setting);                       // setting enum "mode"
extern "C" unsigned velocity_mode; // current value of setting "mode" (+0x30)
D(velocity_ignore_chance_setting);                       // setting num "ignore_chance"
D(velocity_require_forward_setting);                       // setting bool "require_forward"
D(velocity_require_mouse_setting);                       // setting bool "require_mouse"
D(velocity_require_weapon_setting);                       // setting bool "require_weapon"
D(velocity_require_target_setting);                       // setting bool "require_target"
D(velocity_ignore_vertical_setting);                       // setting enum "ignore_vertical"
D(velocity_reduce_multiple_per_tick_setting);                       // setting bool "reduce_multiple_per_tick"
D(velocity_reduce_chance_setting);                       // setting num "reduce_chance"
D(velocity_reduce_delay_setting);                       // setting num "reduce_delay"
D(velocity_reduce_amount_setting);                       // setting num "reduce_amount"
extern "C" float velocity_reduce_amount;    // current value of setting "reduce_amount" (+0x30)
D(velocity_fov_setting);                       // setting num "fov"
D(velocity_jump_chance_setting);                       // setting num "jump_chance"
D(velocity_require_sprinting_jump_setting);                       // setting bool "require_sprinting_jump"

extern "C" void velocity_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_velocity);
    W_enumw(window, gui, velocity_mode_setting);
    switch (velocity_mode) {
    case 0:
        W_slider(window, gui, velocity_reduce_chance_setting);
        W_slider(window, gui, velocity_fov_setting);
        W_slider(window, gui, velocity_reduce_amount_setting);
        W_slider(window, gui, velocity_reduce_delay_setting);
        break;
    case 1:
        W_slider(window, gui, velocity_jump_chance_setting);
        break;
    case 2:
        W_bool(window, gui, velocity_reduce_multiple_per_tick_setting);
        break;
    case 3:
        W_enumw(window, gui, velocity_ignore_vertical_setting);
        W_slider(window, gui, velocity_ignore_chance_setting);
        break;
    }
    W_label(gui, window, str_conditions_in_velocity);
    if (velocity_mode != 2) {
        W_bool(window, gui, velocity_require_target_setting);
        W_bool(window, gui, velocity_require_mouse_setting);
    }
    W_bool(window, gui, velocity_require_forward_setting);
    W_bool(window, gui, velocity_require_weapon_setting);
    if (velocity_mode == 1)
        W_bool(window, gui, velocity_require_sprinting_jump_setting);
    else
        EPILOG_NOP();

}

extern "C" void velocity_register(void *module) {
    R_enum(module, str_mode_in_velocity, velocity_mode_setting);
    R_num(module, str_reduce_chance, velocity_reduce_chance_setting);
    R_num(module, str_fov_in_velocity, velocity_fov_setting);
    R_num(module, str_reduce_amount, velocity_reduce_amount_setting);
    R_num(module, str_reduce_delay_in_velocity, velocity_reduce_delay_setting);
    R_num(module, str_jump_chance, velocity_jump_chance_setting);
    R_bool(module, str_reduce_multiple_per_tick, velocity_reduce_multiple_per_tick_setting);
    R_enum(module, str_ignore_vertical_in_velocity, velocity_ignore_vertical_setting);
    R_num(module, str_ignore_chance, velocity_ignore_chance_setting);
    R_bool(module, str_require_target, velocity_require_target_setting);
    R_bool(module, str_require_mouse, velocity_require_mouse_setting);
    R_bool(module, str_require_forward, velocity_require_forward_setting);
    R_bool(module, str_require_weapon_in_velocity, velocity_require_weapon_setting);
    R_bool(module, str_require_sprinting_jump, velocity_require_sprinting_jump_setting);
}

extern "C" void velocity_status(void *self) {
    if (velocity_mode)
        log_fmt(self, 0x20, str_s_in_velocity, ((const char **)velocity_enum_option_labels_1)[velocity_mode]);
    else
        log_fmt(self, 0x20, str_0f_in_velocity, (double)(velocity_reduce_amount * factor_100));

}
