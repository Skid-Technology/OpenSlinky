// Blink — Simulates teleporting using lag.
// category: utility   descriptor: 0x2ec600   settings block: 0x2ec7f8
// functions: gui blink_gui, register blink_register, status blink_status
// implementation: src/logic/utility/blink.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_blink); // "Main"
S(str_disable_on); // "Disable on"
S(str_direction_in_blink); // "direction"
S(str_max_duration); // "max_duration"
S(str_disable_on_attack); // "disable_on_attack"
S(str_disable_on_block_interact); // "disable_on_block_interact"
S(str_disable_on_block_dig); // "disable_on_block_dig"
S(str_s_in_blink); // "%s"

D(blink_disable_on_attack_setting);                       // setting bool "disable_on_attack"
D(blink_max_duration_setting);                       // setting num "max_duration"
D(blink_enum_option_labels_1);                       // enum option labels (const char *[])
D(blink_direction_setting);                       // setting enum "direction"
extern "C" unsigned blink_direction; // current value of setting "direction" (+0x30)
D(blink_disable_on_block_dig_setting);                       // setting bool "disable_on_block_dig"
D(blink_disable_on_block_interact_setting);                       // setting bool "disable_on_block_interact"

extern "C" void blink_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_blink);
    W_enumw(window, gui, blink_direction_setting);
    W_slider(window, gui, blink_max_duration_setting);
    W_label(gui, window, str_disable_on);
    W_bool(window, gui, blink_disable_on_attack_setting);
    W_bool(window, gui, blink_disable_on_block_interact_setting);
    W_bool(window, gui, blink_disable_on_block_dig_setting);
}

extern "C" void blink_register(void *module) {
    R_enum(module, str_direction_in_blink, blink_direction_setting);
    R_num(module, str_max_duration, blink_max_duration_setting);
    R_bool(module, str_disable_on_attack, blink_disable_on_attack_setting);
    R_bool(module, str_disable_on_block_interact, blink_disable_on_block_interact_setting);
    R_bool(module, str_disable_on_block_dig, blink_disable_on_block_dig_setting);
}

extern "C" void blink_status(void *self) {
    log_fmt(self, 0x20, str_s_in_blink, ((const char **)blink_enum_option_labels_1)[blink_direction]);

}
