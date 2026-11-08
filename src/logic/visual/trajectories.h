// Trajectories — Predicts the trajectory of the item in your hand.
// category: visual   descriptor: 0x2f3df0   settings block: 0x2f3fe8
// functions: gui trajectories_gui, register trajectories_register
// implementation: src/logic/visual/trajectories.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_trajectories); // "Main"
S(str_render_when_holding); // "Render when holding"
S(str_line_width_in_trajectories); // "line_width"
S(str_color_hit); // "color_hit"
S(str_color_miss); // "color_miss"
S(str_render_throwables); // "render_throwables"
S(str_render_pearls); // "render_pearls"
S(str_render_bow); // "render_bow"
S(str_render_uncharged_bow); // "render_uncharged_bow"

D(trajectories_line_width_setting); // setting num "line_width"
D(trajectories_color_hit_setting); // setting key "color_hit"
D(trajectories_color_miss_setting); // setting key "color_miss"
D(trajectories_render_bow_setting); // setting bool "render_bow"
D(trajectories_render_bow); // current value of setting "render_bow" (+0x18)
D(trajectories_render_uncharged_bow_setting); // setting bool "render_uncharged_bow"
D(trajectories_render_pearls_setting); // setting bool "render_pearls"
D(trajectories_render_throwables_setting); // setting bool "render_throwables"

extern "C" void trajectories_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_trajectories);
    W_slider(window, gui, trajectories_line_width_setting);
    W_color(window, gui, trajectories_color_hit_setting);
    W_color(window, gui, trajectories_color_miss_setting);
    W_label(gui, window, str_render_when_holding);
    W_bool(window, gui, trajectories_render_throwables_setting);
    W_bool(window, gui, trajectories_render_pearls_setting);
    W_bool(window, gui, trajectories_render_bow_setting);
    if (*(unsigned char *)trajectories_render_bow == 1) {
        W_bool(window, gui, trajectories_render_uncharged_bow_setting);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void trajectories_register(void *module) {
    R_num(module, str_line_width_in_trajectories, trajectories_line_width_setting);
    R_key(module, str_color_hit, trajectories_color_hit_setting);
    R_key(module, str_color_miss, trajectories_color_miss_setting);
    R_bool(module, str_render_throwables, trajectories_render_throwables_setting);
    R_bool(module, str_render_pearls, trajectories_render_pearls_setting);
    R_bool(module, str_render_bow, trajectories_render_bow_setting);
    R_bool(module, str_render_uncharged_bow, trajectories_render_uncharged_bow_setting);
}
