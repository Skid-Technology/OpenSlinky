// Chest ESP — Highlights chests.
// category: visual   descriptor: 0x2ec908   settings block: 0x2ecb00
// functions: gui chest_esp_gui, register chest_esp_register
// implementation: src/logic/visual/chest_esp.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_chest_esp); // "Main"
S(str_target_in_chest_esp); // "Target"
S(str_customization_in_chest_esp); // "Customization"
S(str_range_in_chest_esp); // "range"
S(str_render_chest); // "render_chest"
S(str_render_ender_chest); // "render_ender_chest"
S(str_render_trapped_chest); // "render_trapped_chest"
S(str_render_beacon); // "render_beacon"
S(str_line_width_in_chest_esp); // "line_width"
S(str_color_chest); // "color_chest"
S(str_color_ender_chest); // "color_ender_chest"
S(str_color_trapped_chest); // "color_trapped_chest"
S(str_color_beacon); // "color_beacon"
S(str_filled_in_chest_esp); // "filled"

D(chest_esp_range_setting);                            // setting num "range"
D(chest_esp_render_chest_setting);                            // setting bool "render_chest"
extern "C" unsigned char chest_esp_render_chest; // current value of setting "render_chest" (+0x18)
D(chest_esp_render_ender_chest_setting);                            // setting bool "render_ender_chest"
extern "C" unsigned char chest_esp_render_ender_chest; // current value of setting "render_ender_chest" (+0x18)
D(chest_esp_render_trapped_chest_setting);                            // setting bool "render_trapped_chest"
extern "C" unsigned char chest_esp_render_trapped_chest; // current value of setting "render_trapped_chest" (+0x18)
D(chest_esp_render_beacon_setting);                            // setting bool "render_beacon"
extern "C" unsigned char chest_esp_render_beacon; // current value of setting "render_beacon" (+0x18)
D(chest_esp_line_width_setting);                            // setting num "line_width"
D(chest_esp_color_ender_chest_setting);                            // setting key "color_ender_chest"
D(chest_esp_color_trapped_chest_setting);                            // setting key "color_trapped_chest"
D(chest_esp_color_beacon_setting);                            // setting key "color_beacon"
D(chest_esp_color_chest_setting);                            // setting key "color_chest"
D(chest_esp_filled_setting);                            // setting bool "filled"

extern "C" void chest_esp_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_chest_esp);
    W_slider(window, gui, chest_esp_range_setting);
    W_label(gui, window, str_target_in_chest_esp);
    W_bool(window, gui, chest_esp_render_chest_setting);
    W_bool(window, gui, chest_esp_render_ender_chest_setting);
    W_bool(window, gui, chest_esp_render_trapped_chest_setting);
    W_bool(window, gui, chest_esp_render_beacon_setting);
    W_label(gui, window, str_customization_in_chest_esp);
    W_slider(window, gui, chest_esp_line_width_setting);
    if (chest_esp_render_chest == 1) goto L1;
    if (chest_esp_render_ender_chest == 1) goto L2;
S2:
    if (chest_esp_render_trapped_chest == 1) goto L3;
S3:
    if (chest_esp_render_beacon == 1) goto L4;
    goto Lend;
L1:
    W_color(window, gui, chest_esp_color_chest_setting);
    if (chest_esp_render_ender_chest != 1) goto S2;
L2:
    W_color(window, gui, chest_esp_color_ender_chest_setting);
    if (chest_esp_render_trapped_chest != 1) goto S3;
L3:
    W_color(window, gui, chest_esp_color_trapped_chest_setting);
    if (chest_esp_render_beacon == 1) goto L4;
    goto Lend;
L4:
    W_color(window, gui, chest_esp_color_beacon_setting);
Lend:
    W_bool(window, gui, chest_esp_filled_setting);
}

extern "C" void chest_esp_register(void *module) {
    R_num(module, str_range_in_chest_esp, chest_esp_range_setting);
    R_bool(module, str_render_chest, chest_esp_render_chest_setting);
    R_bool(module, str_render_ender_chest, chest_esp_render_ender_chest_setting);
    R_bool(module, str_render_trapped_chest, chest_esp_render_trapped_chest_setting);
    R_bool(module, str_render_beacon, chest_esp_render_beacon_setting);
    R_num(module, str_line_width_in_chest_esp, chest_esp_line_width_setting);
    R_key(module, str_color_chest, chest_esp_color_chest_setting);
    R_key(module, str_color_ender_chest, chest_esp_color_ender_chest_setting);
    R_key(module, str_color_trapped_chest, chest_esp_color_trapped_chest_setting);
    R_key(module, str_color_beacon, chest_esp_color_beacon_setting);
    R_bool(module, str_filled_in_chest_esp, chest_esp_filled_setting);
}
