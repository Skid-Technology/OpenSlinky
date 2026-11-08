// Refill — Automatically moves items into your hotbar.
// category: utility   descriptor: 0x2eed90   settings block: 0x2eef88
// functions: gui refill_gui, register refill_register
// implementation: src/logic/utility/refill.cpp
#pragma once
#include "slinky/api.h"

S(str_items_in_refill); // "Items"
S(str_clicks); // "Clicks"
S(str_refill_potion); // "refill_potion"
S(str_refill_potion_non_health); // "refill_potion_non_health"
S(str_refill_soup); // "refill_soup"
S(str_simulate_button_in_refill); // "simulate_button"
S(str_delay_after_open_in_refill); // "delay_after_open"
S(str_delay_before_close_in_refill); // "delay_before_close"
S(str_speed_in_refill); // "speed"
S(str_smart_speed_in_refill); // "smart_speed"

D(refill_delay_before_close_setting); // setting num "delay_before_close"
D(refill_delay_after_open_setting); // setting num "delay_after_open"
D(refill_speed_setting); // setting num "speed"
D(refill_refill_soup_setting); // setting bool "refill_soup"
D(refill_refill_potion_setting); // setting bool "refill_potion"
D(refill_refill_potion); // current value of setting "refill_potion" (+0x18)
D(refill_refill_potion_non_health_setting); // setting bool "refill_potion_non_health"
D(refill_smart_speed_setting); // setting bool "smart_speed"
D(refill_simulate_button_setting); // setting enum "simulate_button"

extern "C" void refill_gui(void *gui, void *window) {
    W_label(gui, window, str_items_in_refill);
    W_bool(window, gui, refill_refill_potion_setting);
    if (*(unsigned char *)refill_refill_potion == 1) {
        W_bool(window, gui, refill_refill_potion_non_health_setting);
    }
    W_bool(window, gui, refill_refill_soup_setting);
    W_label(gui, window, str_clicks);
    W_enumw(window, gui, refill_simulate_button_setting);
    W_slider(window, gui, refill_delay_after_open_setting);
    W_slider(window, gui, refill_delay_before_close_setting);
    W_slider(window, gui, refill_speed_setting);
    W_bool(window, gui, refill_smart_speed_setting);
}

extern "C" void refill_register(void *module) {
    R_bool(module, str_refill_potion, refill_refill_potion_setting);
    R_bool(module, str_refill_potion_non_health, refill_refill_potion_non_health_setting);
    R_bool(module, str_refill_soup, refill_refill_soup_setting);
    R_enum(module, str_simulate_button_in_refill, refill_simulate_button_setting);
    R_num(module, str_delay_after_open_in_refill, refill_delay_after_open_setting);
    R_num(module, str_delay_before_close_in_refill, refill_delay_before_close_setting);
    R_num(module, str_speed_in_refill, refill_speed_setting);
    R_bool(module, str_smart_speed_in_refill, refill_smart_speed_setting);
}
