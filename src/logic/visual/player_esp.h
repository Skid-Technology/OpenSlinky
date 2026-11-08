// Player ESP — Highlights other players.
// category: visual   descriptor: 0x2e9e80   settings block: 0x2ea078
// functions: gui player_esp_gui, register player_esp_register, status player_esp_status
// implementation: src/logic/visual/player_esp.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_player_esp); // "Main"
S(str_customization); // "Customization"
S(str_mode_in_player_esp); // "mode"
S(str_range_in_player_esp); // "range"
S(str_hide_friendlies_in_player_esp); // "hide_friendlies"
S(str_color_mode); // "color_mode"
S(str_color_enemies); // "color_enemies"
S(str_color_friendlies); // "color_friendlies"
S(str_override_target_color_in_player_esp); // "override_target_color"
S(str_color_target); // "color_target"
S(str_line_width_in_player_esp); // "line_width"
S(str_hurt_effect_in_player_esp); // "hurt_effect"
S(str_filled_in_player_esp); // "filled"
S(str_show_2d_box); // "show_2d_box"
S(str_show_health_bar_in_player_esp); // "show_health_bar"
S(str_s_in_player_esp); // "%s"

D(player_esp_line_width_setting);                            // setting num "line_width"
D(player_esp_enum_option_labels_1);                            // enum option labels (const char *[])
D(player_esp_mode_setting);                            // setting enum "mode"
extern "C" unsigned player_esp_mode;      // current value of setting "mode" (+0x30)
D(player_esp_show_2d_box_setting);                            // setting bool "show_2d_box"
extern "C" unsigned char player_esp_show_2d_box; // current value of setting "show_2d_box" (+0x18)
D(player_esp_hurt_effect_setting);                            // setting bool "hurt_effect"
D(player_esp_filled_setting);                            // setting bool "filled"
D(player_esp_show_health_bar_setting);                            // setting bool "show_health_bar"
D(player_esp_range_setting);                            // setting num "range"
D(player_esp_color_target_setting);                            // setting key "color_target"
D(player_esp_color_friendlies_setting);                            // setting key "color_friendlies"
D(player_esp_color_enemies_setting);                            // setting key "color_enemies"
D(player_esp_hide_friendlies_setting);                            // setting bool "hide_friendlies"
extern "C" unsigned char player_esp_hide_friendlies; // current value of setting "hide_friendlies" (+0x18)
D(player_esp_override_target_color_setting);                            // setting bool "override_target_color"
extern "C" unsigned char player_esp_override_target_color; // current value of setting "override_target_color" (+0x18)
D(player_esp_color_mode_setting);                            // setting enum "color_mode"
extern "C" unsigned player_esp_color_mode;      // current value of setting "color_mode" (+0x30)

extern "C" void player_esp_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_player_esp);
    W_slider(window, gui, player_esp_range_setting);
    W_bool(window, gui, player_esp_hide_friendlies_setting);
    W_label(gui, window, str_customization);
    W_enumw(window, gui, player_esp_mode_setting);
    W_enumw(window, gui, player_esp_color_mode_setting);
    if (player_esp_color_mode == 1) {
        W_color(window, gui, player_esp_color_enemies_setting);
        if (player_esp_hide_friendlies == 0)
            W_color(window, gui, player_esp_color_friendlies_setting);
    }
    W_bool(window, gui, player_esp_override_target_color_setting);
    if (player_esp_override_target_color == 1)
        W_color(window, gui, player_esp_color_target_setting);
    W_slider(window, gui, player_esp_line_width_setting);
    W_bool(window, gui, player_esp_show_2d_box_setting);
    if (player_esp_show_2d_box == 1) {
        W_bool(window, gui, player_esp_filled_setting);
        W_bool(window, gui, player_esp_hurt_effect_setting);
    }
    W_bool(window, gui, player_esp_show_health_bar_setting);
}

extern "C" void player_esp_register(void *module) {
    R_enum(module, str_mode_in_player_esp, player_esp_mode_setting);
    R_num(module, str_range_in_player_esp, player_esp_range_setting);
    R_bool(module, str_hide_friendlies_in_player_esp, player_esp_hide_friendlies_setting);
    R_enum(module, str_color_mode, player_esp_color_mode_setting);
    R_key(module, str_color_enemies, player_esp_color_enemies_setting);
    R_key(module, str_color_friendlies, player_esp_color_friendlies_setting);
    R_bool(module, str_override_target_color_in_player_esp, player_esp_override_target_color_setting);
    R_key(module, str_color_target, player_esp_color_target_setting);
    R_num(module, str_line_width_in_player_esp, player_esp_line_width_setting);
    R_bool(module, str_hurt_effect_in_player_esp, player_esp_hurt_effect_setting);
    R_bool(module, str_filled_in_player_esp, player_esp_filled_setting);
    R_bool(module, str_show_2d_box, player_esp_show_2d_box_setting);
    R_bool(module, str_show_health_bar_in_player_esp, player_esp_show_health_bar_setting);
}

extern "C" void player_esp_status(void *self) {
    log_fmt(self, 0x20, str_s_in_player_esp, ((const char **)player_esp_enum_option_labels_1)[player_esp_mode]);

}
