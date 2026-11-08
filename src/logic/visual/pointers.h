// Pointers — Points to other players.
// category: visual   descriptor: 0x2f06c0   settings block: 0x2f08b8
// functions: gui pointers_gui, register pointers_register, status pointers_status
// implementation: src/logic/visual/pointers.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_pointers); // "Main"
S(str_customization_in_pointers); // "Customization"
S(str_range_in_pointers); // "range"
S(str_ignore_fov); // "ignore_fov"
S(str_hide_friendlies_in_pointers); // "hide_friendlies"
S(str_style_in_pointers); // "style"
S(str_color_mode_in_pointers); // "color_mode"
S(str_near_far_range); // "near_far_range"
S(str_color_near); // "color_near"
S(str_color_far); // "color_far"
S(str_color_enemies_in_pointers); // "color_enemies"
S(str_color_friendlies_in_pointers); // "color_friendlies"
S(str_scale_in_pointers); // "scale"
S(str_radius_in_pointers); // "radius"
S(str_scale_radius); // "scale_radius"
S(str_s_in_pointers); // "%s"

D(pointers_near_far_range_setting);                            // setting rng "near_far_range"
D(pointers_scale_setting);                            // setting num "scale"
D(pointers_radius_setting);                            // setting num "radius"
D(pointers_color_near_setting);                            // setting key "color_near"
D(pointers_color_far_setting);                            // setting key "color_far"
D(pointers_enum_option_labels_1);                            // enum option labels (const char *[])
D(pointers_style_setting);                            // setting enum "style"
extern "C" unsigned pointers_style;      // current value of setting "style" (+0x30)
D(pointers_scale_radius_setting);                            // setting bool "scale_radius"
D(pointers_color_mode_setting);                            // setting enum "color_mode"
extern "C" unsigned pointers_color_mode;      // current value of setting "color_mode" (+0x30)
D(pointers_range_setting);                            // setting num "range"
D(pointers_ignore_fov_setting);                            // setting num "ignore_fov"
D(pointers_color_friendlies_setting);                            // setting key "color_friendlies"
D(pointers_color_enemies_setting);                            // setting key "color_enemies"
D(pointers_hide_friendlies_setting);                            // setting bool "hide_friendlies"
extern "C" unsigned char pointers_hide_friendlies; // current value of setting "hide_friendlies" (+0x18)

extern "C" void pointers_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_pointers);
    W_slider(window, gui, pointers_range_setting);
    W_slider(window, gui, pointers_ignore_fov_setting);
    W_bool(window, gui, pointers_hide_friendlies_setting);
    W_label(gui, window, str_customization_in_pointers);
    W_enumw(window, gui, pointers_style_setting);
    W_enumw(window, gui, pointers_color_mode_setting);
    W_range(window, gui, pointers_near_far_range_setting);
    if (pointers_color_mode == 0) {
        W_color(window, gui, pointers_color_near_setting);
        W_color(window, gui, pointers_color_far_setting);
    } else if (pointers_color_mode == 2) {
        W_color(window, gui, pointers_color_enemies_setting);
        if (pointers_hide_friendlies == 0)
            W_color(window, gui, pointers_color_friendlies_setting);
    }
    W_slider(window, gui, pointers_scale_setting);
    W_slider(window, gui, pointers_radius_setting);
    W_bool(window, gui, pointers_scale_radius_setting);
}

extern "C" void pointers_register(void *module) {
    R_num(module, str_range_in_pointers, pointers_range_setting);
    R_num(module, str_ignore_fov, pointers_ignore_fov_setting);
    R_bool(module, str_hide_friendlies_in_pointers, pointers_hide_friendlies_setting);
    R_enum(module, str_style_in_pointers, pointers_style_setting);
    R_enum(module, str_color_mode_in_pointers, pointers_color_mode_setting);
    R_rng(module, str_near_far_range, pointers_near_far_range_setting);
    R_key(module, str_color_near, pointers_color_near_setting);
    R_key(module, str_color_far, pointers_color_far_setting);
    R_key(module, str_color_enemies_in_pointers, pointers_color_enemies_setting);
    R_key(module, str_color_friendlies_in_pointers, pointers_color_friendlies_setting);
    R_num(module, str_scale_in_pointers, pointers_scale_setting);
    R_num(module, str_radius_in_pointers, pointers_radius_setting);
    R_bool(module, str_scale_radius, pointers_scale_radius_setting);
}

extern "C" void pointers_status(void *self) {
    log_fmt(self, 0x20, str_s_in_pointers, ((const char **)pointers_enum_option_labels_1)[pointers_style]);

}
