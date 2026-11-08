// Block In — Places blocks around you.
// category: block   descriptor: 0x2f6108   settings block: 0x2f6300
// functions: gui block_in_gui, register block_in_register
// implementation: src/logic/block/block_in.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_block_in); // "Main"
S(str_conditions_in_block_in); // "Conditions"
S(str_speed_in_block_in); // "speed"
S(str_require_ground); // "require_ground"

D(block_in_require_ground_setting); // setting bool "require_ground"
D(block_in_speed_setting); // setting num "speed"

extern "C" void block_in_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_block_in);
    W_slider(window, gui, block_in_speed_setting);
    W_label(gui, window, str_conditions_in_block_in);
    W_bool(window, gui, block_in_require_ground_setting);
}

extern "C" void block_in_register(void *module) {
    R_num(module, str_speed_in_block_in, block_in_speed_setting);
    R_bool(module, str_require_ground, block_in_require_ground_setting);
}
