// No Slow — Removes slowdown while using items.
// category: move   descriptor: 0x2f2c90   settings block: 0x2f2e88
// functions: gui no_slow_gui, register no_slow_register
// implementation: src/logic/move/no_slow.cpp
#pragma once
#include "slinky/api.h"

S(str_retained_speed_in_no_slow); // "Retained speed"
S(str_mult_sword); // "mult_sword"
S(str_mult_bow); // "mult_bow"
S(str_mult_consumables); // "mult_consumables"

D(no_slow_mult_bow_setting); // setting num "mult_bow"
D(no_slow_mult_sword_setting); // setting num "mult_sword"
D(no_slow_mult_consumables_setting); // setting num "mult_consumables"

extern "C" void no_slow_gui(void *gui, void *window) {
    W_label(gui, window, str_retained_speed_in_no_slow);
    W_slider(window, gui, no_slow_mult_sword_setting);
    W_slider(window, gui, no_slow_mult_bow_setting);
    W_slider(window, gui, no_slow_mult_consumables_setting);
}

extern "C" void no_slow_register(void *module) {
    R_num(module, str_mult_sword, no_slow_mult_sword_setting);
    R_num(module, str_mult_bow, no_slow_mult_bow_setting);
    R_num(module, str_mult_consumables, no_slow_mult_consumables_setting);
}
