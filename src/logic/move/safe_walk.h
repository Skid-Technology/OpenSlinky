// Safe Walk — Prevents you from walking off ledges.
// category: move   descriptor: 0x2f2030   settings block: 0x2f2228
// functions: gui safe_walk_gui, register safe_walk_register
// implementation: src/logic/move/safe_walk.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_safe_walk); // "Main"
S(str_conditions_in_safe_walk); // "Conditions"
S(str_motion_multiplier); // "motion_multiplier"
S(str_require_holding_blocks_in_safe_walk); // "require_holding_blocks"
S(str_require_looking_down_in_safe_walk); // "require_looking_down"
S(str_require_not_forward_in_safe_walk); // "require_not_forward"

D(safe_walk_motion_multiplier_setting); // setting num "motion_multiplier"
D(safe_walk_require_looking_down_setting); // setting bool "require_looking_down"
D(safe_walk_require_not_forward_setting); // setting bool "require_not_forward"
D(safe_walk_require_holding_blocks_setting); // setting bool "require_holding_blocks"

extern "C" void safe_walk_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_safe_walk);
    W_slider(window, gui, safe_walk_motion_multiplier_setting);
    W_label(gui, window, str_conditions_in_safe_walk);
    W_bool(window, gui, safe_walk_require_holding_blocks_setting);
    W_bool(window, gui, safe_walk_require_looking_down_setting);
    W_bool(window, gui, safe_walk_require_not_forward_setting);
}

extern "C" void safe_walk_register(void *module) {
    R_num(module, str_motion_multiplier, safe_walk_motion_multiplier_setting);
    R_bool(module, str_require_holding_blocks_in_safe_walk, safe_walk_require_holding_blocks_setting);
    R_bool(module, str_require_looking_down_in_safe_walk, safe_walk_require_looking_down_setting);
    R_bool(module, str_require_not_forward_in_safe_walk, safe_walk_require_not_forward_setting);
}
