// Backtrack — Uses lag to make players easier to hit.
// category: combat   descriptor: 0x2edad0   settings block: 0x2edcc8
// functions: gui backtrack_gui, register backtrack_register, status backtrack_status
// implementation: src/logic/combat/backtrack.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_backtrack); // "Main"
S(str_real_position_indicator); // "Real position indicator"
S(str_conditions_in_backtrack); // "Conditions"
S(str_target_dist); // "target_dist"
S(str_delay_in_backtrack); // "delay"
S(str_max_hurt_time_in_backtrack); // "max_hurt_time"
S(str_cooldown_in_backtrack); // "cooldown"
S(str_disable_on_hit); // "disable_on_hit"
S(str_render_remote_pos); // "render_remote_pos"
S(str_indicator_color); // "indicator_color"
S(str_indicator_line_width); // "indicator_line_width"
S(str_indicator_filled); // "indicator_filled"
S(str_indicator_rotate); // "indicator_rotate"
S(str_require_mouse_pressed_in_backtrack); // "require_mouse_pressed"
S(str_require_weapon_in_backtrack); // "require_weapon"
S(str_0fms); // "%.0fms"

D(backtrack_indicator_line_width_setting);                    // setting num "indicator_line_width"
D(backtrack_indicator_color_setting);                    // setting key "indicator_color"
D(backtrack_indicator_rotate_setting);                    // setting bool "indicator_rotate"
D(backtrack_indicator_filled_setting);                    // setting bool "indicator_filled"
D(backtrack_max_hurt_time_setting);                    // setting num "max_hurt_time"
D(backtrack_target_dist_setting);                    // setting rng "target_dist"
D(backtrack_delay_setting);                    // setting num "delay"
extern "C" float backtrack_delay; // current value of setting "delay" (+0x30)
D(backtrack_cooldown_setting);                    // setting num "cooldown"
D(backtrack_require_mouse_pressed_setting);                    // setting bool "require_mouse_pressed"
D(backtrack_require_weapon_setting);                    // setting bool "require_weapon"
D(backtrack_disable_on_hit_setting);                    // setting bool "disable_on_hit"
D(backtrack_render_remote_pos_setting);                    // setting bool "render_remote_pos"
D(backtrack_render_remote_pos);                    // current value of setting "render_remote_pos" (+0x18)

extern "C" void backtrack_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_backtrack);
    W_range(window, gui, backtrack_target_dist_setting);
    W_slider(window, gui, backtrack_delay_setting);
    W_slider(window, gui, backtrack_max_hurt_time_setting);
    W_slider(window, gui, backtrack_cooldown_setting);
    W_bool(window, gui, backtrack_disable_on_hit_setting);
    W_label(gui, window, str_real_position_indicator);
    W_bool(window, gui, backtrack_render_remote_pos_setting);
    if (*(unsigned char *)backtrack_render_remote_pos == 1) {
        W_color(window, gui, backtrack_indicator_color_setting);
        W_slider(window, gui, backtrack_indicator_line_width_setting);
        W_bool(window, gui, backtrack_indicator_filled_setting);
        W_bool(window, gui, backtrack_indicator_rotate_setting);
    }
    W_label(gui, window, str_conditions_in_backtrack);
    W_bool(window, gui, backtrack_require_mouse_pressed_setting);
    W_bool(window, gui, backtrack_require_weapon_setting);
}

extern "C" void backtrack_register(void *module) {
    R_rng(module, str_target_dist, backtrack_target_dist_setting);
    R_num(module, str_delay_in_backtrack, backtrack_delay_setting);
    R_num(module, str_max_hurt_time_in_backtrack, backtrack_max_hurt_time_setting);
    R_num(module, str_cooldown_in_backtrack, backtrack_cooldown_setting);
    R_bool(module, str_disable_on_hit, backtrack_disable_on_hit_setting);
    R_bool(module, str_render_remote_pos, backtrack_render_remote_pos_setting);
    R_key(module, str_indicator_color, backtrack_indicator_color_setting);
    R_num(module, str_indicator_line_width, backtrack_indicator_line_width_setting);
    R_bool(module, str_indicator_filled, backtrack_indicator_filled_setting);
    R_bool(module, str_indicator_rotate, backtrack_indicator_rotate_setting);
    R_bool(module, str_require_mouse_pressed_in_backtrack, backtrack_require_mouse_pressed_setting);
    R_bool(module, str_require_weapon_in_backtrack, backtrack_require_weapon_setting);
}

extern "C" void backtrack_status(void *self) {
    log_fmt(self, 0x20, str_0fms, (double)backtrack_delay);

}
