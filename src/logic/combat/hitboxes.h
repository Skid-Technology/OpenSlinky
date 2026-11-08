// Hitboxes — Expands hitboxes of other players.
// category: combat   descriptor: 0x2ecca8   settings block: 0x2ecea0
// functions: gui hitboxes_gui, register hitboxes_register, status hitboxes_status
// implementation: src/logic/combat/hitboxes.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_hitboxes); // "Main"
S(str_expand_in_hitboxes); // "expand"
S(str_2f_in_hitboxes); // "%.2f"

D(hitboxes_expand_setting);                    // setting num "expand"
extern "C" float hitboxes_expand; // current value of setting "expand" (+0x30)

extern "C" void hitboxes_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_hitboxes);
    W_slider(window, gui, hitboxes_expand_setting);
}

extern "C" void hitboxes_register(void *module) {
    R_num(module, str_expand_in_hitboxes, hitboxes_expand_setting);
}

extern "C" void hitboxes_status(void *self) {
    log_fmt(self, 0x20, str_2f_in_hitboxes, (double)hitboxes_expand);

}
