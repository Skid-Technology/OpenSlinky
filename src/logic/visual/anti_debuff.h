// Anti Debuff — Removes negative visual effects.
// category: visual   descriptor: 0x2f3860   settings block: 0x2f3a58
// functions: gui anti_debuff_gui, register anti_debuff_register
// implementation: src/logic/visual/anti_debuff.cpp
#pragma once
#include "slinky/api.h"

S(str_effects); // "Effects"
S(str_blindness_in_anti_debuff); // "blindness"
S(str_nausea_in_anti_debuff); // "nausea"

D(anti_debuff_blindness_setting); // setting bool "blindness"
D(anti_debuff_nausea_setting); // setting bool "nausea"

extern "C" void anti_debuff_gui(void *gui, void *window) {
    W_label(gui, window, str_effects);
    W_bool(window, gui, anti_debuff_blindness_setting);
    W_bool(window, gui, anti_debuff_nausea_setting);
}

extern "C" void anti_debuff_register(void *module) {
    R_bool(module, str_blindness_in_anti_debuff, anti_debuff_blindness_setting);
    R_bool(module, str_nausea_in_anti_debuff, anti_debuff_nausea_setting);
}
