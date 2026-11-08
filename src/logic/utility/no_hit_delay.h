// No Hit Delay — Removes 1.8 swing delay.
// category: utility   descriptor: 0x2f0b70   settings block: 0x2f0d68
// functions: gui no_hit_delay_gui, register no_hit_delay_register, status no_hit_delay_status
// implementation: src/logic/utility/no_hit_delay.cpp
#pragma once
#include "slinky/api.h"

S(str_in_game_in_no_hit_delay); // "In-game"
S(str_mode_in_no_hit_delay); // "mode"
S(str_s_in_no_hit_delay); // "%s"

D(no_hit_delay_enum_option_labels_1);                       // enum option labels (const char *[])
D(no_hit_delay_mode_setting);                       // setting enum "mode"
extern "C" unsigned no_hit_delay_mode; // current value of setting "mode" (+0x30)

extern "C" void no_hit_delay_gui(void *gui, void *window) {
    W_label(gui, window, str_in_game_in_no_hit_delay);
    W_enumw(window, gui, no_hit_delay_mode_setting);
}

extern "C" void no_hit_delay_register(void *module) {
    R_enum(module, str_mode_in_no_hit_delay, no_hit_delay_mode_setting);
}

extern "C" void no_hit_delay_status(void *self) {
    log_fmt(self, 0x20, str_s_in_no_hit_delay, ((const char **)no_hit_delay_enum_option_labels_1)[no_hit_delay_mode]);

}
