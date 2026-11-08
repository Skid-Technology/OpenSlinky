// Fake Lag — Adds an artificial delay to your packets.
// category: utility   descriptor: 0x2ebcc0   settings block: 0x2ebeb8
// functions: gui fake_lag_gui, register fake_lag_register, status fake_lag_status
// implementation: src/logic/utility/fake_lag.cpp
#pragma once
#include "slinky/api.h"

S(str_delay); // "Delay"
S(str_conditions_in_fake_lag); // "Conditions"
S(str_mode_in_fake_lag); // "mode"
S(str_inbound_in_fake_lag); // "inbound"
S(str_outbound_in_fake_lag); // "outbound"
S(str_realtime_damage_in_fake_lag); // "realtime_damage"
S(str_require_weapon_in_fake_lag); // "require_weapon"
S(str_require_attack); // "require_attack"
S(str_require_ingame); // "require_ingame"
S(str_s_in_fake_lag); // "%s"

D(fake_lag_inbound_setting);                       // setting num "inbound"
D(fake_lag_outbound_setting);                       // setting num "outbound"
D(fake_lag_realtime_damage_setting);                       // setting bool "realtime_damage"
D(fake_lag_enum_option_labels_1);                       // enum option labels (const char *[])
D(fake_lag_mode_setting);                       // setting enum "mode"
extern "C" unsigned fake_lag_mode; // current value of setting "mode" (+0x30)
D(fake_lag_require_ingame_setting);                       // setting bool "require_ingame"
D(fake_lag_require_weapon_setting);                       // setting bool "require_weapon"
D(fake_lag_require_attack_setting);                       // setting bool "require_attack"

extern "C" void fake_lag_gui(void *gui, void *window) {
    W_label(gui, window, str_delay);
    W_enumw(window, gui, fake_lag_mode_setting);
    W_slider(window, gui, fake_lag_inbound_setting);
    W_slider(window, gui, fake_lag_outbound_setting);
    W_bool(window, gui, fake_lag_realtime_damage_setting);
    W_label(gui, window, str_conditions_in_fake_lag);
    W_bool(window, gui, fake_lag_require_weapon_setting);
    W_bool(window, gui, fake_lag_require_attack_setting);
    W_bool(window, gui, fake_lag_require_ingame_setting);
}

extern "C" void fake_lag_register(void *module) {
    R_enum(module, str_mode_in_fake_lag, fake_lag_mode_setting);
    R_num(module, str_inbound_in_fake_lag, fake_lag_inbound_setting);
    R_num(module, str_outbound_in_fake_lag, fake_lag_outbound_setting);
    R_bool(module, str_realtime_damage_in_fake_lag, fake_lag_realtime_damage_setting);
    R_bool(module, str_require_weapon_in_fake_lag, fake_lag_require_weapon_setting);
    R_bool(module, str_require_attack, fake_lag_require_attack_setting);
    R_bool(module, str_require_ingame, fake_lag_require_ingame_setting);
}

extern "C" void fake_lag_status(void *self) {
    log_fmt(self, 0x20, str_s_in_fake_lag, ((const char **)fake_lag_enum_option_labels_1)[fake_lag_mode]);

}
