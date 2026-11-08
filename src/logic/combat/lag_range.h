// Lag Range — Uses lag to stay out of target range.
// category: combat   descriptor: 0x2ef9c0   settings block: 0x2efbb8
// functions: gui lag_range_gui, register lag_range_register, status lag_range_status
// implementation: src/logic/combat/lag_range.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_lag_range); // "Main"
S(str_flush_if); // "Flush if"
S(str_real_position_indicator_in_lag_range); // "Real position indicator"
S(str_conditions_in_lag_range); // "Conditions"
S(str_delay_in_lag_range); // "delay"
S(str_flush_sprint_reset); // "flush_sprint_reset"
S(str_flush_splash_potion); // "flush_splash_potion"
S(str_render_remote_pos_in_lag_range); // "render_remote_pos"
S(str_indicator_color_in_lag_range); // "indicator_color"
S(str_indicator_line_width_in_lag_range); // "indicator_line_width"
S(str_indicator_filled_in_lag_range); // "indicator_filled"
S(str_require_weapon_in_lag_range); // "require_weapon"
S(str_0fms_in_lag_range); // "%.0fms"

D(lag_range_indicator_line_width_setting);                    // setting num "indicator_line_width"
D(lag_range_indicator_color_setting);                    // setting key "indicator_color"
D(lag_range_indicator_filled_setting);                    // setting bool "indicator_filled"
D(lag_range_delay_setting);                    // setting num "delay"
extern "C" float lag_range_delay; // current value of setting "delay" (+0x30)
D(lag_range_require_weapon_setting);                    // setting bool "require_weapon"
D(lag_range_flush_splash_potion_setting);                    // setting bool "flush_splash_potion"
D(lag_range_flush_sprint_reset_setting);                    // setting bool "flush_sprint_reset"
D(lag_range_render_remote_pos_setting);                    // setting bool "render_remote_pos"
D(lag_range_render_remote_pos);                    // current value of setting "render_remote_pos" (+0x18)

extern "C" void lag_range_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_lag_range);
    W_slider(window, gui, lag_range_delay_setting);
    W_label(gui, window, str_flush_if);
    W_bool(window, gui, lag_range_flush_sprint_reset_setting);
    W_bool(window, gui, lag_range_flush_splash_potion_setting);
    W_label(gui, window, str_real_position_indicator_in_lag_range);
    W_bool(window, gui, lag_range_render_remote_pos_setting);
    if (*(unsigned char *)lag_range_render_remote_pos == 1) {
        W_color(window, gui, lag_range_indicator_color_setting);
        W_slider(window, gui, lag_range_indicator_line_width_setting);
        W_bool(window, gui, lag_range_indicator_filled_setting);
    }
    W_label(gui, window, str_conditions_in_lag_range);
    W_bool(window, gui, lag_range_require_weapon_setting);
}

extern "C" void lag_range_register(void *module) {
    R_num(module, str_delay_in_lag_range, lag_range_delay_setting);
    R_bool(module, str_flush_sprint_reset, lag_range_flush_sprint_reset_setting);
    R_bool(module, str_flush_splash_potion, lag_range_flush_splash_potion_setting);
    R_bool(module, str_render_remote_pos_in_lag_range, lag_range_render_remote_pos_setting);
    R_key(module, str_indicator_color_in_lag_range, lag_range_indicator_color_setting);
    R_num(module, str_indicator_line_width_in_lag_range, lag_range_indicator_line_width_setting);
    R_bool(module, str_indicator_filled_in_lag_range, lag_range_indicator_filled_setting);
    R_bool(module, str_require_weapon_in_lag_range, lag_range_require_weapon_setting);
}

extern "C" void lag_range_status(void *self) {
    log_fmt(self, 0x20, str_0fms_in_lag_range, (double)lag_range_delay);

}
