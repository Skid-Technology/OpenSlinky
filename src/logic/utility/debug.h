// Debug — Prints debug information in chat.
// category: utility   descriptor: 0x2f4430   settings block: 0x2f4628
// functions: gui debug_gui, register debug_register
// implementation: src/logic/utility/debug.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_debug); // "Main"
S(str_clicks_in_debug); // "clicks"
S(str_hurt_time_in_debug); // "hurt_time"
S(str_item_use_state); // "item_use_state"

D(debug_hurt_time_setting); // setting bool "hurt_time"
D(debug_item_use_state_setting); // setting bool "item_use_state"
D(debug_clicks_setting); // setting bool "clicks"

extern "C" void debug_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_debug);
    W_bool(window, gui, debug_clicks_setting);
    W_bool(window, gui, debug_hurt_time_setting);
    W_bool(window, gui, debug_item_use_state_setting);
}

extern "C" void debug_register(void *module) {
    R_bool(module, str_clicks_in_debug, debug_clicks_setting);
    R_bool(module, str_hurt_time_in_debug, debug_hurt_time_setting);
    R_bool(module, str_item_use_state, debug_item_use_state_setting);
}
