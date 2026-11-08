// No Use Delay — Removes vanilla delay when consuming items.
// category: utility   descriptor: 0x2f1210   settings block: 0x2f1408
// functions: gui no_use_delay_gui, register no_use_delay_register
// implementation: src/logic/utility/no_use_delay.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_no_use_delay); // "Main"
S(str_allow_desync); // "allow_desync"

D(no_use_delay_allow_desync_setting); // setting bool "allow_desync"

extern "C" void no_use_delay_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_no_use_delay);
    W_bool(window, gui, no_use_delay_allow_desync_setting);
}

extern "C" void no_use_delay_register(void *module) {
    R_bool(module, str_allow_desync, no_use_delay_allow_desync_setting);
}
