// Piercing — Allows hitting entities through obstructions.
// category: combat   descriptor: 0x2f3ab0   settings block: 0x2f3ca8
// functions: gui piercing_gui, register piercing_register
// implementation: src/logic/combat/piercing.cpp
#pragma once
#include "slinky/api.h"

S(str_always_hit_through); // "Always hit through"
S(str_conditions_in_piercing); // "Conditions"
S(str_sort_by_in_piercing); // "sort_by"
S(str_non_players); // "non_players"
S(str_friendlies_in_piercing); // "friendlies"
S(str_bots_in_piercing); // "bots"
S(str_enemies_in_piercing); // "enemies"
S(str_blocks_in_piercing); // "blocks"
S(str_require_weapon_in_piercing); // "require_weapon"
S(str_require_inside_bb); // "require_inside_bb"

D(piercing_require_inside_bb_setting); // setting bool "require_inside_bb"
D(piercing_require_weapon_setting); // setting bool "require_weapon"
D(piercing_non_players_setting); // setting bool "non_players"
D(piercing_friendlies_setting); // setting bool "friendlies"
D(piercing_bots_setting); // setting bool "bots"
D(piercing_enemies_setting); // setting bool "enemies"
D(piercing_sort_by_setting); // setting enum "sort_by"
D(piercing_blocks_setting); // setting bool "blocks"

extern "C" void piercing_gui(void *gui, void *window) {
    W_label(gui, window, str_always_hit_through);
    W_enumw(window, gui, piercing_sort_by_setting);
    W_bool(window, gui, piercing_non_players_setting);
    W_bool(window, gui, piercing_friendlies_setting);
    W_bool(window, gui, piercing_bots_setting);
    W_bool(window, gui, piercing_enemies_setting);
    W_bool(window, gui, piercing_blocks_setting);
    W_label(gui, window, str_conditions_in_piercing);
    W_bool(window, gui, piercing_require_weapon_setting);
    W_bool(window, gui, piercing_require_inside_bb_setting);
}

extern "C" void piercing_register(void *module) {
    R_enum(module, str_sort_by_in_piercing, piercing_sort_by_setting);
    R_bool(module, str_non_players, piercing_non_players_setting);
    R_bool(module, str_friendlies_in_piercing, piercing_friendlies_setting);
    R_bool(module, str_bots_in_piercing, piercing_bots_setting);
    R_bool(module, str_enemies_in_piercing, piercing_enemies_setting);
    R_bool(module, str_blocks_in_piercing, piercing_blocks_setting);
    R_bool(module, str_require_weapon_in_piercing, piercing_require_weapon_setting);
    R_bool(module, str_require_inside_bb, piercing_require_inside_bb_setting);
}
