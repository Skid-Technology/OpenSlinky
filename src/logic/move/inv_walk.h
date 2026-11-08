// Inv Walk — Allows moving in inventory.
// category: move   descriptor: 0x2ef3d0   settings block: 0x2ef5c8
// functions: gui inv_walk_gui, register inv_walk_register, status inv_walk_status
// implementation: src/logic/move/inv_walk.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_inv_walk); // "Main"
S(str_other_containers); // "Other containers"
S(str_mode_in_inv_walk); // "mode"
S(str_stop_on_click); // "stop_on_click"
S(str_stop_duration_in_inv_walk); // "stop_duration"
S(str_stop_non_inv); // "stop_non_inv"
S(str_s_in_inv_walk); // "%s"

D(inv_walk_enum_option_labels_1);                       // enum option labels (const char *[])
D(inv_walk_mode_setting);                       // setting enum "mode"
extern "C" unsigned inv_walk_mode; // current value of setting "mode" (+0x30)
D(inv_walk_stop_on_click_setting);                       // setting enum "stop_on_click"
extern "C" unsigned inv_walk_stop_on_click; // current value of setting "stop_on_click" (+0x30)
D(inv_walk_stop_duration_setting);                       // setting num "stop_duration"
D(inv_walk_stop_non_inv_setting);                       // setting enum "stop_non_inv"

extern "C" void inv_walk_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_inv_walk);
    W_enumw(window, gui, inv_walk_mode_setting);
    if (inv_walk_mode != 0) {
        W_enumw(window, gui, inv_walk_stop_on_click_setting);
        if (inv_walk_stop_on_click != 2)
            W_slider(window, gui, inv_walk_stop_duration_setting);
        W_label(gui, window, str_other_containers);
        W_enumw(window, gui, inv_walk_stop_non_inv_setting);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void inv_walk_register(void *module) {
    R_enum(module, str_mode_in_inv_walk, inv_walk_mode_setting);
    R_enum(module, str_stop_on_click, inv_walk_stop_on_click_setting);
    R_num(module, str_stop_duration_in_inv_walk, inv_walk_stop_duration_setting);
    R_enum(module, str_stop_non_inv, inv_walk_stop_non_inv_setting);
}

extern "C" void inv_walk_status(void *self) {
    log_fmt(self, 0x20, str_s_in_inv_walk, ((const char **)inv_walk_enum_option_labels_1)[inv_walk_mode]);

}
