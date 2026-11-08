// Auto Pot — Uses healing potions for you.
// category: utility   descriptor: 0x2ef128   settings block: 0x2ef320
// functions: gui auto_pot_gui, register auto_pot_register
// implementation: src/logic/utility/auto_pot.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_auto_pot); // "Main"
S(str_delay_in_auto_pot); // "delay"
S(str_prevent_waste); // "prevent_waste"
S(str_double_splash); // "double_splash"
S(str_use_non_health); // "use_non_health"

D(auto_pot_use_non_health_setting); // setting bool "use_non_health"
D(auto_pot_delay_setting); // setting num "delay"
D(auto_pot_prevent_waste_setting); // setting bool "prevent_waste"
D(auto_pot_double_splash_setting); // setting bool "double_splash"

extern "C" void auto_pot_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_auto_pot);
    W_slider(window, gui, auto_pot_delay_setting);
    W_bool(window, gui, auto_pot_prevent_waste_setting);
    W_bool(window, gui, auto_pot_double_splash_setting);
    W_bool(window, gui, auto_pot_use_non_health_setting);
}

extern "C" void auto_pot_register(void *module) {
    R_num(module, str_delay_in_auto_pot, auto_pot_delay_setting);
    R_bool(module, str_prevent_waste, auto_pot_prevent_waste_setting);
    R_bool(module, str_double_splash, auto_pot_double_splash_setting);
    R_bool(module, str_use_non_health, auto_pot_use_non_health_setting);
}
