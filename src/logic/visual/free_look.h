// Free Look — Freely move the camera around.
// category: visual   descriptor: 0x2edef0   settings block: 0x2ee0e8
// functions: gui free_look_gui, register free_look_register
// implementation: src/logic/visual/free_look.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_free_look); // "Main"
S(str_target_perspective); // "target_perspective"

D(free_look_target_perspective_setting); // setting enum "target_perspective"

extern "C" void free_look_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_free_look);
    W_enumw(window, gui, free_look_target_perspective_setting);
}

extern "C" void free_look_register(void *module) {
    R_enum(module, str_target_perspective, free_look_target_perspective_setting);
}
