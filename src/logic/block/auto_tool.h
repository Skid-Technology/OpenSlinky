// Auto Tool — Selects the best tool when digging.
// category: block   descriptor: 0x2ee158   settings block: 0x2ee350
// functions: gui auto_tool_gui, register auto_tool_register
// implementation: src/logic/block/auto_tool.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_auto_tool); // "Main"
S(str_allow_while_using); // "Allow while using"
S(str_conditions_in_auto_tool); // "Conditions"
S(str_activation_time_in_auto_tool); // "activation_time"
S(str_switch_back); // "switch_back"
S(str_allow_using_sword); // "allow_using_sword"
S(str_allow_using_tools); // "allow_using_tools"
S(str_allow_using_fists); // "allow_using_fists"
S(str_allow_using_other); // "allow_using_other"
S(str_require_sneak_in_auto_tool); // "require_sneak"
S(str_restrict_allowed_blocks_in_auto_tool); // "restrict_allowed_blocks"
S(str_allowed_blocks_in_auto_tool); // "allowed_blocks"

D(auto_tool_activation_time_setting); // setting num "activation_time"
D(auto_tool_allow_using_fists_setting); // setting bool "allow_using_fists"
D(auto_tool_allow_using_sword_setting); // setting bool "allow_using_sword"
D(auto_tool_allow_using_tools_setting); // setting bool "allow_using_tools"
D(auto_tool_allow_using_other_setting); // setting bool "allow_using_other"
D(auto_tool_require_sneak_setting); // setting bool "require_sneak"
D(auto_tool_restrict_allowed_blocks_setting); // setting bool "restrict_allowed_blocks"
D(auto_tool_restrict_allowed_blocks); // current value of setting "restrict_allowed_blocks" (+0x18)
D(auto_tool_switch_back_setting); // setting bool "switch_back"
D(auto_tool_allowed_blocks_setting); // setting kind2 "allowed_blocks"

extern "C" void auto_tool_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_auto_tool);
    W_slider(window, gui, auto_tool_activation_time_setting);
    W_bool(window, gui, auto_tool_switch_back_setting);
    W_label(gui, window, str_allow_while_using);
    W_bool(window, gui, auto_tool_allow_using_sword_setting);
    W_bool(window, gui, auto_tool_allow_using_tools_setting);
    W_bool(window, gui, auto_tool_allow_using_fists_setting);
    W_bool(window, gui, auto_tool_allow_using_other_setting);
    W_label(gui, window, str_conditions_in_auto_tool);
    W_bool(window, gui, auto_tool_require_sneak_setting);
    W_bool(window, gui, auto_tool_restrict_allowed_blocks_setting);
    if (*(unsigned char *)auto_tool_restrict_allowed_blocks == 1) {
        W_items(window, gui, auto_tool_allowed_blocks_setting);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void auto_tool_register(void *module) {
    R_num(module, str_activation_time_in_auto_tool, auto_tool_activation_time_setting);
    R_bool(module, str_switch_back, auto_tool_switch_back_setting);
    R_bool(module, str_allow_using_sword, auto_tool_allow_using_sword_setting);
    R_bool(module, str_allow_using_tools, auto_tool_allow_using_tools_setting);
    R_bool(module, str_allow_using_fists, auto_tool_allow_using_fists_setting);
    R_bool(module, str_allow_using_other, auto_tool_allow_using_other_setting);
    R_bool(module, str_require_sneak_in_auto_tool, auto_tool_require_sneak_setting);
    R_bool(module, str_restrict_allowed_blocks_in_auto_tool, auto_tool_restrict_allowed_blocks_setting);
    R_kind2(module, str_allowed_blocks_in_auto_tool, auto_tool_allowed_blocks_setting);
}
