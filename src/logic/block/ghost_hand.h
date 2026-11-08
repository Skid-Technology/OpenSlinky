// Ghost Hand — Allows interacting with blocks through entities.
// category: block   descriptor: 0x2f48b0   settings block: 0x2f4aa8
// functions: gui ghost_hand_gui, register ghost_hand_register
// implementation: src/logic/block/ghost_hand.cpp
#pragma once
#include "slinky/api.h"

S(str_interact_through); // "Interact through"
S(str_allow_while_using_in_ghost_hand); // "Allow while using"
S(str_conditions_in_ghost_hand); // "Conditions"
S(str_non_players_in_ghost_hand); // "non_players"
S(str_friendlies_in_ghost_hand); // "friendlies"
S(str_bots_in_ghost_hand); // "bots"
S(str_enemies_in_ghost_hand); // "enemies"
S(str_allow_using_sword_in_ghost_hand); // "allow_using_sword"
S(str_allow_using_tools_in_ghost_hand); // "allow_using_tools"
S(str_allow_using_fists_in_ghost_hand); // "allow_using_fists"
S(str_allow_using_buckets); // "allow_using_buckets"
S(str_allow_using_fns); // "allow_using_fns"
S(str_allow_using_cobweb); // "allow_using_cobweb"
S(str_allow_using_other_in_ghost_hand); // "allow_using_other"
S(str_require_mouse_pressed_in_ghost_hand); // "require_mouse_pressed"

D(ghost_hand_require_mouse_pressed_setting); // setting bool "require_mouse_pressed"
D(ghost_hand_allow_using_fists_setting); // setting bool "allow_using_fists"
D(ghost_hand_allow_using_sword_setting); // setting bool "allow_using_sword"
D(ghost_hand_allow_using_tools_setting); // setting bool "allow_using_tools"
D(ghost_hand_allow_using_buckets_setting); // setting bool "allow_using_buckets"
D(ghost_hand_allow_using_fns_setting); // setting bool "allow_using_fns"
D(ghost_hand_allow_using_cobweb_setting); // setting bool "allow_using_cobweb"
D(ghost_hand_allow_using_other_setting); // setting bool "allow_using_other"
D(ghost_hand_non_players_setting); // setting bool "non_players"
D(ghost_hand_bots_setting); // setting bool "bots"
D(ghost_hand_friendlies_setting); // setting bool "friendlies"
D(ghost_hand_enemies_setting); // setting bool "enemies"

extern "C" void ghost_hand_gui(void *gui, void *window) {
    W_label(gui, window, str_interact_through);
    W_bool(window, gui, ghost_hand_non_players_setting);
    W_bool(window, gui, ghost_hand_friendlies_setting);
    W_bool(window, gui, ghost_hand_bots_setting);
    W_bool(window, gui, ghost_hand_enemies_setting);
    W_label(gui, window, str_allow_while_using_in_ghost_hand);
    W_bool(window, gui, ghost_hand_allow_using_sword_setting);
    W_bool(window, gui, ghost_hand_allow_using_tools_setting);
    W_bool(window, gui, ghost_hand_allow_using_fists_setting);
    W_bool(window, gui, ghost_hand_allow_using_buckets_setting);
    W_bool(window, gui, ghost_hand_allow_using_fns_setting);
    W_bool(window, gui, ghost_hand_allow_using_cobweb_setting);
    W_bool(window, gui, ghost_hand_allow_using_other_setting);
    W_label(gui, window, str_conditions_in_ghost_hand);
    W_bool(window, gui, ghost_hand_require_mouse_pressed_setting);
}

extern "C" void ghost_hand_register(void *module) {
    R_bool(module, str_non_players_in_ghost_hand, ghost_hand_non_players_setting);
    R_bool(module, str_friendlies_in_ghost_hand, ghost_hand_friendlies_setting);
    R_bool(module, str_bots_in_ghost_hand, ghost_hand_bots_setting);
    R_bool(module, str_enemies_in_ghost_hand, ghost_hand_enemies_setting);
    R_bool(module, str_allow_using_sword_in_ghost_hand, ghost_hand_allow_using_sword_setting);
    R_bool(module, str_allow_using_tools_in_ghost_hand, ghost_hand_allow_using_tools_setting);
    R_bool(module, str_allow_using_fists_in_ghost_hand, ghost_hand_allow_using_fists_setting);
    R_bool(module, str_allow_using_buckets, ghost_hand_allow_using_buckets_setting);
    R_bool(module, str_allow_using_fns, ghost_hand_allow_using_fns_setting);
    R_bool(module, str_allow_using_cobweb, ghost_hand_allow_using_cobweb_setting);
    R_bool(module, str_allow_using_other_in_ghost_hand, ghost_hand_allow_using_other_setting);
    R_bool(module, str_require_mouse_pressed_in_ghost_hand, ghost_hand_require_mouse_pressed_setting);
}
