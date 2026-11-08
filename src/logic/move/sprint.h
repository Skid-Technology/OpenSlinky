// Sprint — Automatically sprints for you.
// category: move   descriptor: 0x2ed840   settings block: 0x2eda38
// functions: gui sprint_gui, register sprint_register
// implementation: src/logic/move/sprint.cpp
#pragma once
#include "slinky/api.h"

S(str_allow_while); // "Allow while"
S(str_while_using_item); // "while_using_item"
S(str_while_backwards); // "while_backwards"
S(str_while_sideways); // "while_sideways"
S(str_while_in_gui); // "while_in_gui"

D(sprint_while_in_gui_setting); // setting bool "while_in_gui"
D(sprint_while_using_item_setting); // setting bool "while_using_item"
D(sprint_while_backwards_setting); // setting bool "while_backwards"
D(sprint_while_sideways_setting); // setting bool "while_sideways"

extern "C" void sprint_gui(void *gui, void *window) {
    W_label(gui, window, str_allow_while);
    W_bool(window, gui, sprint_while_using_item_setting);
    W_bool(window, gui, sprint_while_backwards_setting);
    W_bool(window, gui, sprint_while_sideways_setting);
    W_bool(window, gui, sprint_while_in_gui_setting);
}

extern "C" void sprint_register(void *module) {
    R_bool(module, str_while_using_item, sprint_while_using_item_setting);
    R_bool(module, str_while_backwards, sprint_while_backwards_setting);
    R_bool(module, str_while_sideways, sprint_while_sideways_setting);
    R_bool(module, str_while_in_gui, sprint_while_in_gui_setting);
}
