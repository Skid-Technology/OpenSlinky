// Anti Bot — Ignores NPCs for specific modules.
// category: utility   descriptor: 0x2eacd0   settings block: 0x2eaec8
// functions: gui anti_bot_gui, register anti_bot_register
// implementation: src/logic/utility/anti_bot.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_anti_bot); // "Main"
S(str_invalid_uuid_in_anti_bot); // "invalid_uuid"
S(str_invalid_username_in_anti_bot); // "invalid_username"
S(str_always_invis); // "always_invis"
S(str_always_stationary_in_anti_bot); // "always_stationary"
S(str_sleeping_in_anti_bot); // "sleeping"
S(str_min_age); // "min_age"

D(anti_bot_min_age_setting); // setting num "min_age"
D(anti_bot_always_stationary_setting); // setting bool "always_stationary"
D(anti_bot_invalid_uuid_setting); // setting enum "invalid_uuid"
D(anti_bot_sleeping_setting); // setting bool "sleeping"
D(anti_bot_always_invis_setting); // setting bool "always_invis"
D(anti_bot_invalid_username_setting); // setting bool "invalid_username"

extern "C" void anti_bot_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_anti_bot);
    W_enumw(window, gui, anti_bot_invalid_uuid_setting);
    W_bool(window, gui, anti_bot_invalid_username_setting);
    W_bool(window, gui, anti_bot_always_invis_setting);
    W_bool(window, gui, anti_bot_always_stationary_setting);
    W_bool(window, gui, anti_bot_sleeping_setting);
    W_slider(window, gui, anti_bot_min_age_setting);
}

extern "C" void anti_bot_register(void *module) {
    R_enum(module, str_invalid_uuid_in_anti_bot, anti_bot_invalid_uuid_setting);
    R_bool(module, str_invalid_username_in_anti_bot, anti_bot_invalid_username_setting);
    R_bool(module, str_always_invis, anti_bot_always_invis_setting);
    R_bool(module, str_always_stationary_in_anti_bot, anti_bot_always_stationary_setting);
    R_bool(module, str_sleeping_in_anti_bot, anti_bot_sleeping_setting);
    R_num(module, str_min_age, anti_bot_min_age_setting);
}
