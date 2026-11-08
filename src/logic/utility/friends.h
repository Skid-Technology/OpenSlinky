// Friends — Lets you mark other players as friendly.
// category: utility   descriptor: 0x2f0038   settings block: 0x2f0230
// functions: gui friends_gui, register friends_register
// implementation: src/logic/utility/friends.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_friends); // "Main"
S(str_middle_click_add); // "middle_click_add"
S(str_friends_second); // "friends"

D(friends_middle_click_add_setting); // setting bool "middle_click_add"
D(friends_friends_setting); // setting kind2 "friends"

extern "C" void friends_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_friends);
    W_bool(window, gui, friends_middle_click_add_setting);
}

extern "C" void friends_register(void *module) {
    R_bool(module, str_middle_click_add, friends_middle_click_add_setting);
    R_kind2(module, str_friends_second, friends_friends_setting);
}
