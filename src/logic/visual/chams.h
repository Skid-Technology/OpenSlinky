// Chams — Renders players behind blocks.
// category: visual   descriptor: 0x2f0fe0   settings block: 0x2f11d8
// functions: gui chams_gui, register chams_register
// implementation: src/logic/visual/chams.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_chams); // "Main"
S(str_hide_friendlies_in_chams); // "hide_friendlies"

D(chams_hide_friendlies_setting); // setting bool "hide_friendlies"

extern "C" void chams_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_chams);
    W_bool(window, gui, chams_hide_friendlies_setting);
}

extern "C" void chams_register(void *module) {
    R_bool(module, str_hide_friendlies_in_chams, chams_hide_friendlies_setting);
}
