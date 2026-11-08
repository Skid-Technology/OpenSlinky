// Mod Overlay — Displays a list of all active mods.
// category: visual   descriptor: 0x2ecef0   settings block: 0x2ed0e8
// functions: gui mod_overlay_gui, register mod_overlay_register
// implementation: src/logic/visual/mod_overlay.cpp
#pragma once
#include "slinky/api.h"

S(str_visible_categories); // "Visible categories"
S(str_customization_in_mod_overlay); // "Customization"
S(str_conditions_in_mod_overlay); // "Conditions"
S(str_show_combat); // "show_combat"
S(str_show_move); // "show_move"
S(str_show_block); // "show_block"
S(str_show_visual); // "show_visual"
S(str_show_utility); // "show_utility"
S(str_text_size_in_mod_overlay); // "text_size"
S(str_padding_x_in_mod_overlay); // "padding_x"
S(str_padding_y_in_mod_overlay); // "padding_y"
S(str_bg_rounding); // "bg_rounding"
S(str_name_color); // "name_color"
S(str_details_color_in_mod_overlay); // "details_color"
S(str_lowercase_in_mod_overlay); // "lowercase"
S(str_show_details_in_mod_overlay); // "show_details"
S(str_require_bound); // "require_bound"

D(mod_overlay_show_combat_setting); // setting bool "show_combat"
D(mod_overlay_show_move_setting); // setting bool "show_move"
D(mod_overlay_show_block_setting); // setting bool "show_block"
D(mod_overlay_show_visual_setting); // setting bool "show_visual"
D(mod_overlay_show_utility_setting); // setting bool "show_utility"
D(mod_overlay_require_bound_setting); // setting bool "require_bound"
D(mod_overlay_lowercase_setting); // setting bool "lowercase"
D(mod_overlay_show_details_setting); // setting bool "show_details"
D(mod_overlay_text_size_setting); // setting num "text_size"
D(mod_overlay_name_color_setting); // setting key "name_color"
D(mod_overlay_details_color_setting); // setting key "details_color"
D(mod_overlay_bg_rounding_setting); // setting num "bg_rounding"
D(mod_overlay_padding_x_setting); // setting num "padding_x"
D(mod_overlay_padding_y_setting); // setting num "padding_y"

extern "C" void mod_overlay_gui(void *gui, void *window) {
    W_label(gui, window, str_visible_categories);
    W_bool(window, gui, mod_overlay_show_combat_setting);
    W_bool(window, gui, mod_overlay_show_move_setting);
    W_bool(window, gui, mod_overlay_show_block_setting);
    W_bool(window, gui, mod_overlay_show_visual_setting);
    W_bool(window, gui, mod_overlay_show_utility_setting);
    W_label(gui, window, str_customization_in_mod_overlay);
    W_slider(window, gui, mod_overlay_text_size_setting);
    W_slider(window, gui, mod_overlay_padding_x_setting);
    W_slider(window, gui, mod_overlay_padding_y_setting);
    W_slider(window, gui, mod_overlay_bg_rounding_setting);
    W_color(window, gui, mod_overlay_name_color_setting);
    W_color(window, gui, mod_overlay_details_color_setting);
    W_bool(window, gui, mod_overlay_lowercase_setting);
    W_bool(window, gui, mod_overlay_show_details_setting);
    W_label(gui, window, str_conditions_in_mod_overlay);
    W_bool(window, gui, mod_overlay_require_bound_setting);
}

extern "C" void mod_overlay_register(void *module) {
    R_bool(module, str_show_combat, mod_overlay_show_combat_setting);
    R_bool(module, str_show_move, mod_overlay_show_move_setting);
    R_bool(module, str_show_block, mod_overlay_show_block_setting);
    R_bool(module, str_show_visual, mod_overlay_show_visual_setting);
    R_bool(module, str_show_utility, mod_overlay_show_utility_setting);
    R_num(module, str_text_size_in_mod_overlay, mod_overlay_text_size_setting);
    R_num(module, str_padding_x_in_mod_overlay, mod_overlay_padding_x_setting);
    R_num(module, str_padding_y_in_mod_overlay, mod_overlay_padding_y_setting);
    R_num(module, str_bg_rounding, mod_overlay_bg_rounding_setting);
    R_key(module, str_name_color, mod_overlay_name_color_setting);
    R_key(module, str_details_color_in_mod_overlay, mod_overlay_details_color_setting);
    R_bool(module, str_lowercase_in_mod_overlay, mod_overlay_lowercase_setting);
    R_bool(module, str_show_details_in_mod_overlay, mod_overlay_show_details_setting);
    R_bool(module, str_require_bound, mod_overlay_require_bound_setting);
}
