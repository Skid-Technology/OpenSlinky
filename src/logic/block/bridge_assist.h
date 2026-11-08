// Bridge Assist — Sneaks and unsneaks for you while bridging.
// category: block   descriptor: 0x2e9ac0   settings block: 0x2e9cb8
// functions: gui bridge_assist_gui, register bridge_assist_register
// implementation: src/logic/block/bridge_assist.cpp
#pragma once
#include "slinky/api.h"

S(str_main); // "Main"
S(str_conditions_in_bridge_assist); // "Conditions"
S(str_edge_offset_in_bridge_assist); // "edge_offset"
S(str_unsneak_delay_in_bridge_assist); // "unsneak_delay"
S(str_select_blocks_in_bridge_assist); // "select_blocks"
S(str_randomize_in_bridge_assist); // "randomize"
S(str_sneak_on_jump_in_bridge_assist); // "sneak_on_jump"
S(str_avoid_double_sneak); // "avoid_double_sneak"
S(str_require_sneak); // "require_sneak"
S(str_require_holding_blocks); // "require_holding_blocks"
S(str_require_looking_down); // "require_looking_down"
S(str_require_not_forward); // "require_not_forward"

D(bridge_assist_require_sneak_setting); // setting bool "require_sneak"
D(bridge_assist_require_looking_down_setting); // setting bool "require_looking_down"
D(bridge_assist_require_not_forward_setting); // setting bool "require_not_forward"
D(bridge_assist_select_blocks_setting); // setting enum "select_blocks"
D(bridge_assist_require_holding_blocks_setting); // setting bool "require_holding_blocks"
D(bridge_assist_sneak_on_jump_setting); // setting bool "sneak_on_jump"
D(bridge_assist_edge_offset_setting); // setting num "edge_offset"
D(bridge_assist_randomize_setting); // setting bool "randomize"
D(bridge_assist_avoid_double_sneak_setting); // setting bool "avoid_double_sneak"
D(bridge_assist_unsneak_delay_setting); // setting num "unsneak_delay"

extern "C" void bridge_assist_gui(void *gui, void *window) {
    W_label(gui, window, str_main);
    W_slider(window, gui, bridge_assist_edge_offset_setting);
    W_slider(window, gui, bridge_assist_unsneak_delay_setting);
    W_enumw(window, gui, bridge_assist_select_blocks_setting);
    W_bool(window, gui, bridge_assist_randomize_setting);
    W_bool(window, gui, bridge_assist_sneak_on_jump_setting);
    W_bool(window, gui, bridge_assist_avoid_double_sneak_setting);
    W_label(gui, window, str_conditions_in_bridge_assist);
    W_bool(window, gui, bridge_assist_require_sneak_setting);
    W_bool(window, gui, bridge_assist_require_holding_blocks_setting);
    W_bool(window, gui, bridge_assist_require_looking_down_setting);
    W_bool(window, gui, bridge_assist_require_not_forward_setting);
}

extern "C" void bridge_assist_register(void *module) {
    R_num(module, str_edge_offset_in_bridge_assist, bridge_assist_edge_offset_setting);
    R_num(module, str_unsneak_delay_in_bridge_assist, bridge_assist_unsneak_delay_setting);
    R_enum(module, str_select_blocks_in_bridge_assist, bridge_assist_select_blocks_setting);
    R_bool(module, str_randomize_in_bridge_assist, bridge_assist_randomize_setting);
    R_bool(module, str_sneak_on_jump_in_bridge_assist, bridge_assist_sneak_on_jump_setting);
    R_bool(module, str_avoid_double_sneak, bridge_assist_avoid_double_sneak_setting);
    R_bool(module, str_require_sneak, bridge_assist_require_sneak_setting);
    R_bool(module, str_require_holding_blocks, bridge_assist_require_holding_blocks_setting);
    R_bool(module, str_require_looking_down, bridge_assist_require_looking_down_setting);
    R_bool(module, str_require_not_forward, bridge_assist_require_not_forward_setting);
}
