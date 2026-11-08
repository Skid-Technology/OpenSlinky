// Auto Soup — Uses soups for you.
// category: utility   descriptor: 0x2e96b0   settings block: 0x2e98a8
// functions: gui auto_soup_gui, register auto_soup_register
// implementation: src/logic/utility/auto_soup.cpp
#pragma once
#include "slinky/api.h"

S(str_maximum_health); // "Maximum health"
S(str_delays); // "Delays"
S(str_extra); // "Extra"
S(str_single_soup_max_hp); // "single_soup_max_hp"
S(str_double_soup_max_hp); // "double_soup_max_hp"
S(str_randomization_in_auto_soup); // "randomization"
S(str_delay_before); // "delay_before"
S(str_delay_between); // "delay_between"
S(str_delay_after); // "delay_after"
S(str_drop_bowls_in_auto_soup); // "drop_bowls"
S(str_override_item); // "override_item"
S(str_allowed_items); // "allowed_items"

D(auto_soup_single_soup_max_hp_setting);                            // setting num "single_soup_max_hp"
extern "C" float auto_soup_single_soup_max_hp;         // current value of setting "single_soup_max_hp" (+0x30)
D(auto_soup_delay_before_setting);                            // setting num "delay_before"
D(auto_soup_double_soup_max_hp_setting);                            // setting num "double_soup_max_hp"
extern "C" float auto_soup_double_soup_max_hp;         // current value of setting "double_soup_max_hp" (+0x30)
D(auto_soup_delay_between_setting);                            // setting num "delay_between"
D(auto_soup_delay_after_setting);                            // setting num "delay_after"
D(auto_soup_randomization_setting);                            // setting num "randomization"
D(auto_soup_override_item_setting);                            // setting bool "override_item"
extern "C" unsigned char auto_soup_override_item; // current value of setting "override_item" (+0x18)
D(auto_soup_drop_bowls_setting);                            // setting bool "drop_bowls"
D(auto_soup_allowed_items_setting);                            // setting kind2 "allowed_items"

extern "C" void auto_soup_gui(void *gui, void *window) {
    W_label(gui, window, str_maximum_health);
    W_slider(window, gui, auto_soup_single_soup_max_hp_setting);
    float lo = auto_soup_single_soup_max_hp;
    if (auto_soup_double_soup_max_hp > lo)
        auto_soup_double_soup_max_hp = lo;
    W_slider(window, gui, auto_soup_double_soup_max_hp_setting);
    W_label(gui, window, str_delays);
    W_slider(window, gui, auto_soup_randomization_setting);
    W_slider(window, gui, auto_soup_delay_before_setting);
    W_slider(window, gui, auto_soup_delay_between_setting);
    W_slider(window, gui, auto_soup_delay_after_setting);
    W_label(gui, window, str_extra);
    W_bool(window, gui, auto_soup_drop_bowls_setting);
    W_bool(window, gui, auto_soup_override_item_setting);
    if (auto_soup_override_item == 1)
        W_items(window, gui, auto_soup_allowed_items_setting);
    else
        EPILOG_NOP();

}

extern "C" void auto_soup_register(void *module) {
    R_num(module, str_single_soup_max_hp, auto_soup_single_soup_max_hp_setting);
    R_num(module, str_double_soup_max_hp, auto_soup_double_soup_max_hp_setting);
    R_num(module, str_randomization_in_auto_soup, auto_soup_randomization_setting);
    R_num(module, str_delay_before, auto_soup_delay_before_setting);
    R_num(module, str_delay_between, auto_soup_delay_between_setting);
    R_num(module, str_delay_after, auto_soup_delay_after_setting);
    R_bool(module, str_drop_bowls_in_auto_soup, auto_soup_drop_bowls_setting);
    R_bool(module, str_override_item, auto_soup_override_item_setting);
    R_kind2(module, str_allowed_items, auto_soup_allowed_items_setting);
}
