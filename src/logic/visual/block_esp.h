// Block ESP — Highlights blocks.
// category: visual   descriptor: 0x2f1740   settings block: 0x2f1938
// functions: gui block_esp_gui, register block_esp_register
// implementation: src/logic/visual/block_esp.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_block_esp); // "Main"
S(str_block_render_limit); // "Block render limit"
S(str_customization_in_block_esp); // "Customization"
S(str_range_in_block_esp); // "range"
S(str_allowed_blocks_in_block_esp); // "allowed_blocks"
S(str_should_limit_block_render); // "should_limit_block_render"
S(str_block_render_limit_in_block_esp); // "block_render_limit"
S(str_line_width_in_block_esp); // "line_width"

D(block_esp_allowed_blocks_setting); // setting kind2 "allowed_blocks"
D(block_esp_line_width_setting); // setting num "line_width"
D(block_esp_block_render_limit_setting); // setting num "block_render_limit"
D(block_esp_range_setting); // setting num "range"
D(block_esp_should_limit_block_render_setting); // setting bool "should_limit_block_render"
D(block_esp_should_limit_block_render); // current value of setting "should_limit_block_render" (+0x18)

extern "C" void block_esp_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_block_esp);
    W_slider(window, gui, block_esp_range_setting);
    W_items(window, gui, block_esp_allowed_blocks_setting);
    W_label(gui, window, str_block_render_limit);
    W_bool(window, gui, block_esp_should_limit_block_render_setting);
    if (*(unsigned char *)block_esp_should_limit_block_render == 1) {
        W_slider(window, gui, block_esp_block_render_limit_setting);
    }
    W_label(gui, window, str_customization_in_block_esp);
    W_slider(window, gui, block_esp_line_width_setting);
}

extern "C" void block_esp_register(void *module) {
    R_num(module, str_range_in_block_esp, block_esp_range_setting);
    R_kind2(module, str_allowed_blocks_in_block_esp, block_esp_allowed_blocks_setting);
    R_bool(module, str_should_limit_block_render, block_esp_should_limit_block_render_setting);
    R_num(module, str_block_render_limit_in_block_esp, block_esp_block_render_limit_setting);
    R_num(module, str_line_width_in_block_esp, block_esp_line_width_setting);
}
