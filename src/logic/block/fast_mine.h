// Fast Mine — Increases block mining speed.
// category: block   descriptor: 0x2f1d90   settings block: 0x2f1f88
// functions: gui fast_mine_gui, register fast_mine_register, status fast_mine_status
// implementation: src/logic/block/fast_mine.cpp
#pragma once
#include "slinky/api.h"

S(str_2fx_in_fast_mine); // "%.2fx"
S(str_main_in_fast_mine); // "Main"
S(str_speed_in_fast_mine); // "speed"
S(str_break_delay_in_fast_mine); // "break_delay"
S(str_decr_block_hit_delay); // "decr_block_hit_delay"

D(fast_mine_break_delay_setting);                    // setting num "break_delay"
D(fast_mine_decr_block_hit_delay_setting);                    // setting bool "decr_block_hit_delay"
D(fast_mine_speed_setting);                    // setting num "speed"
extern "C" float fast_mine_speed; // current value of setting "speed" (+0x30)

extern "C" void fast_mine_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_fast_mine);
    W_slider(window, gui, fast_mine_speed_setting);
    W_slider(window, gui, fast_mine_break_delay_setting);
    W_bool(window, gui, fast_mine_decr_block_hit_delay_setting);
}

extern "C" void fast_mine_register(void *module) {
    R_num(module, str_speed_in_fast_mine, fast_mine_speed_setting);
    R_num(module, str_break_delay_in_fast_mine, fast_mine_break_delay_setting);
    R_bool(module, str_decr_block_hit_delay, fast_mine_decr_block_hit_delay_setting);
}

extern "C" void fast_mine_status(void *self) {
    log_fmt(self, 0x20, str_2fx_in_fast_mine, (double)fast_mine_speed);

}
