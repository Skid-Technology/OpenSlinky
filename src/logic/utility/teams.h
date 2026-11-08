// Teams — Ignores teammates for specific modules.
// category: utility   descriptor: 0x2eb400   settings block: 0x2eb5f8
// functions: gui teams_gui, register teams_register
// implementation: src/logic/utility/teams.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_teams); // "Main"
S(str_match_self_color); // "match_self_color"
S(str_team_colors); // "team_colors"

D(teams_match_self_color_setting); // setting bool "match_self_color"
D(teams_team_colors_setting); // setting kind2 "team_colors"

extern "C" void teams_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_teams);
    W_bool(window, gui, teams_match_self_color_setting);
    W_items(window, gui, teams_team_colors_setting);
}

extern "C" void teams_register(void *module) {
    R_bool(module, str_match_self_color, teams_match_self_color_setting);
    R_kind2(module, str_team_colors, teams_team_colors_setting);
}
