// GUI — Graphical interface for the cheat.
// category: visual   descriptor: 0x2eb740   settings block: 0x2eb938
// functions: gui gui_gui, register gui_register
// implementation: src/logic/visual/gui.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_gui); // "Main"
S(str_layout); // "Layout"
S(str_scale_in_gui); // "scale"
S(str_accent_color_in_gui); // "accent_color"
S(str_allow_input); // "allow_input"
S(str_compact_in_gui); // "compact"
S(str_wide_in_gui); // "wide"

D(gui_accent_color_setting); // setting key "accent_color"
D(gui_scale_setting); // setting enum "scale"
D(gui_allow_input_setting); // setting bool "allow_input"
D(gui_compact_setting); // setting bool "compact"
D(gui_wide_setting); // setting bool "wide"

extern "C" void gui_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_gui);
    W_enumw(window, gui, gui_scale_setting);
    W_color(window, gui, gui_accent_color_setting);
    W_bool(window, gui, gui_allow_input_setting);
    W_label(gui, window, str_layout);
    W_bool(window, gui, gui_compact_setting);
    W_bool(window, gui, gui_wide_setting);
}

extern "C" void gui_register(void *module) {
    R_enum(module, str_scale_in_gui, gui_scale_setting);
    R_key(module, str_accent_color_in_gui, gui_accent_color_setting);
    R_bool(module, str_allow_input, gui_allow_input_setting);
    R_bool(module, str_compact_in_gui, gui_compact_setting);
    R_bool(module, str_wide_in_gui, gui_wide_setting);
}
