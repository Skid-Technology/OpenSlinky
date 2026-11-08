// Indicators — Points to incoming projectiles.
// category: visual   descriptor: 0x2f1440   settings block: 0x2f1638
// functions: gui indicators_gui, register indicators_register
// implementation: src/logic/visual/indicators.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_indicators); // "Main"
S(str_warn_for); // "Warn for"
S(str_conditions_in_indicators); // "Conditions"
S(str_scale_in_indicators); // "scale"
S(str_radius_in_indicators); // "radius"
S(str_show_arrow); // "show_arrow"
S(str_show_fireball); // "show_fireball"
S(str_show_pearl); // "show_pearl"
S(str_require_coming_closer); // "require_coming_closer"

D(indicators_scale_setting); // setting num "scale"
D(indicators_radius_setting); // setting num "radius"
D(indicators_show_arrow_setting); // setting bool "show_arrow"
D(indicators_show_fireball_setting); // setting bool "show_fireball"
D(indicators_show_pearl_setting); // setting bool "show_pearl"
D(indicators_require_coming_closer_setting); // setting bool "require_coming_closer"

extern "C" void indicators_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_indicators);
    W_slider(window, gui, indicators_scale_setting);
    W_slider(window, gui, indicators_radius_setting);
    W_label(gui, window, str_warn_for);
    W_bool(window, gui, indicators_show_arrow_setting);
    W_bool(window, gui, indicators_show_fireball_setting);
    W_bool(window, gui, indicators_show_pearl_setting);
    W_label(gui, window, str_conditions_in_indicators);
    W_bool(window, gui, indicators_require_coming_closer_setting);
}

extern "C" void indicators_register(void *module) {
    R_num(module, str_scale_in_indicators, indicators_scale_setting);
    R_num(module, str_radius_in_indicators, indicators_radius_setting);
    R_bool(module, str_show_arrow, indicators_show_arrow_setting);
    R_bool(module, str_show_fireball, indicators_show_fireball_setting);
    R_bool(module, str_show_pearl, indicators_show_pearl_setting);
    R_bool(module, str_require_coming_closer, indicators_require_coming_closer_setting);
}
