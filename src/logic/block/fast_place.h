// Fast Place — Allows placing blocks faster.
// category: block   descriptor: 0x2ea2d0   settings block: 0x2ea4c8
// functions: gui fast_place_gui, register fast_place_register, status fast_place_status
// implementation: src/logic/block/fast_place.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_fast_place); // "Main"
S(str_max_cps); // "max_cps"
S(str_activate_delay_in_fast_place); // "activate_delay"
S(str_1f_in_fast_place); // "%.1f"

D(fast_place_max_cps_setting);                    // setting num "max_cps"
extern "C" float fast_place_max_cps; // current value of setting "max_cps" (+0x30)
D(fast_place_activate_delay_setting);                    // setting num "activate_delay"

extern "C" void fast_place_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_fast_place);
    W_slider(window, gui, fast_place_max_cps_setting);
    W_slider(window, gui, fast_place_activate_delay_setting);
}

extern "C" void fast_place_register(void *module) {
    R_num(module, str_max_cps, fast_place_max_cps_setting);
    R_num(module, str_activate_delay_in_fast_place, fast_place_activate_delay_setting);
}

extern "C" void fast_place_status(void *self) {
    log_fmt(self, 0x20, str_1f_in_fast_place, (double)fast_place_max_cps);

}
