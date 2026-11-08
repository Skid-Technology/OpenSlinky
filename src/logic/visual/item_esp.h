// Item ESP — Highlights dropped items.
// category: visual   descriptor: 0x2f1a80   settings block: 0x2f1c78
// functions: gui item_esp_gui, register item_esp_register
// implementation: src/logic/visual/item_esp.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_item_esp); // "Main"
S(str_customization_in_item_esp); // "Customization"
S(str_range_in_item_esp); // "range"
S(str_show_count_in_item_esp); // "show_count"
S(str_allowed_items_in_item_esp); // "allowed_items"
S(str_highlight_all); // "highlight_all"
S(str_default_color_in_item_esp); // "default_color"

D(item_esp_show_count_setting); // setting bool "show_count"
D(item_esp_range_setting); // setting num "range"
D(item_esp_default_color_setting); // setting key "default_color"
D(item_esp_highlight_all_setting); // setting bool "highlight_all"
D(item_esp_highlight_all); // current value of setting "highlight_all" (+0x18)
D(item_esp_allowed_items_setting); // setting kind2 "allowed_items"

extern "C" void item_esp_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_item_esp);
    W_slider(window, gui, item_esp_range_setting);
    W_label(gui, window, str_customization_in_item_esp);
    W_bool(window, gui, item_esp_show_count_setting);
    W_items(window, gui, item_esp_allowed_items_setting);
    W_bool(window, gui, item_esp_highlight_all_setting);
    if (*(unsigned char *)item_esp_highlight_all == 1) {
        W_color(window, gui, item_esp_default_color_setting);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void item_esp_register(void *module) {
    R_num(module, str_range_in_item_esp, item_esp_range_setting);
    R_bool(module, str_show_count_in_item_esp, item_esp_show_count_setting);
    R_kind2(module, str_allowed_items_in_item_esp, item_esp_allowed_items_setting);
    R_bool(module, str_highlight_all, item_esp_highlight_all_setting);
    R_key(module, str_default_color_in_item_esp, item_esp_default_color_setting);
}
