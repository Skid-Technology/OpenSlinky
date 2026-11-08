// Clutch — Clutches for you when falling.
// category: block   descriptor: 0x2f5580   settings block: 0x2f5778
// functions: gui clutch_gui, register clutch_register
// implementation: src/logic/block/clutch.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_clutch); // "Main"
S(str_aim_in_clutch); // "Aim"
S(str_snap_back); // "Snap-back"
S(str_conditions_in_clutch); // "Conditions"
S(str_range_in_clutch); // "range"
S(str_max_aim_dist); // "max_aim_dist"
S(str_min_height); // "min_height"
S(str_click_speed_in_clutch); // "click_speed"
S(str_randomization_in_clutch); // "randomization"
S(str_select_blocks_in_clutch); // "select_blocks"
S(str_sideways_only); // "sideways_only"
S(str_aim_speed); // "aim_speed"
S(str_aim_mult); // "aim_mult"
S(str_aim_mult_strength); // "aim_mult_strength"
S(str_aim_multipoint); // "aim_multipoint"
S(str_snapback_delay); // "snapback_delay"
S(str_snapback_duration); // "snapback_duration"
S(str_snapback_instant_on_jump); // "snapback_instant_on_jump"
S(str_snapback_disable_after); // "snapback_disable_after"
S(str_require_air_in_clutch); // "require_air"
S(str_require_damage_in_clutch); // "require_damage"
S(str_require_backwards); // "require_backwards"

D(clutch_click_speed_setting); // setting num "click_speed"
D(clutch_snapback_duration_setting); // setting num "snapback_duration"
D(clutch_snapback_delay_setting); // setting num "snapback_delay"
D(clutch_randomization_setting); // setting num "randomization"
D(clutch_aim_speed_setting); // setting num "aim_speed"
D(clutch_aim_mult_setting); // setting num "aim_mult"
D(clutch_aim_mult_strength_setting); // setting num "aim_mult_strength"
D(clutch_snapback_instant_on_jump_setting); // setting bool "snapback_instant_on_jump"
D(clutch_require_air_setting); // setting bool "require_air"
D(clutch_require_damage_setting); // setting bool "require_damage"
D(clutch_require_backwards_setting); // setting bool "require_backwards"
D(clutch_min_height_setting); // setting num "min_height"
D(clutch_range_setting); // setting num "range"
D(clutch_max_aim_dist_setting); // setting num "max_aim_dist"
D(clutch_sideways_only_setting); // setting bool "sideways_only"
D(clutch_aim_multipoint_setting); // setting bool "aim_multipoint"
D(clutch_select_blocks_setting); // setting enum "select_blocks"
D(clutch_snapback_disable_after_setting); // setting bool "snapback_disable_after"

extern "C" void clutch_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_clutch);
    W_slider(window, gui, clutch_range_setting);
    W_slider(window, gui, clutch_max_aim_dist_setting);
    W_slider(window, gui, clutch_min_height_setting);
    W_slider(window, gui, clutch_click_speed_setting);
    W_slider(window, gui, clutch_randomization_setting);
    W_enumw(window, gui, clutch_select_blocks_setting);
    W_bool(window, gui, clutch_sideways_only_setting);
    W_label(gui, window, str_aim_in_clutch);
    W_slider(window, gui, clutch_aim_speed_setting);
    W_slider(window, gui, clutch_aim_mult_setting);
    W_slider(window, gui, clutch_aim_mult_strength_setting);
    W_bool(window, gui, clutch_aim_multipoint_setting);
    W_label(gui, window, str_snap_back);
    W_slider(window, gui, clutch_snapback_delay_setting);
    W_slider(window, gui, clutch_snapback_duration_setting);
    W_bool(window, gui, clutch_snapback_instant_on_jump_setting);
    W_bool(window, gui, clutch_snapback_disable_after_setting);
    W_label(gui, window, str_conditions_in_clutch);
    W_bool(window, gui, clutch_require_air_setting);
    W_bool(window, gui, clutch_require_damage_setting);
    W_bool(window, gui, clutch_require_backwards_setting);
}

extern "C" void clutch_register(void *module) {
    R_num(module, str_range_in_clutch, clutch_range_setting);
    R_num(module, str_max_aim_dist, clutch_max_aim_dist_setting);
    R_num(module, str_min_height, clutch_min_height_setting);
    R_num(module, str_click_speed_in_clutch, clutch_click_speed_setting);
    R_num(module, str_randomization_in_clutch, clutch_randomization_setting);
    R_enum(module, str_select_blocks_in_clutch, clutch_select_blocks_setting);
    R_bool(module, str_sideways_only, clutch_sideways_only_setting);
    R_num(module, str_aim_speed, clutch_aim_speed_setting);
    R_num(module, str_aim_mult, clutch_aim_mult_setting);
    R_num(module, str_aim_mult_strength, clutch_aim_mult_strength_setting);
    R_bool(module, str_aim_multipoint, clutch_aim_multipoint_setting);
    R_num(module, str_snapback_delay, clutch_snapback_delay_setting);
    R_num(module, str_snapback_duration, clutch_snapback_duration_setting);
    R_bool(module, str_snapback_instant_on_jump, clutch_snapback_instant_on_jump_setting);
    R_bool(module, str_snapback_disable_after, clutch_snapback_disable_after_setting);
    R_bool(module, str_require_air_in_clutch, clutch_require_air_setting);
    R_bool(module, str_require_damage_in_clutch, clutch_require_damage_setting);
    R_bool(module, str_require_backwards, clutch_require_backwards_setting);
}
