// Hit Select — Perfectly times your hits.
// category: combat   descriptor: 0x2ee4f0   settings block: 0x2ee6e8
// functions: gui hit_select_gui, register hit_select_register, status hit_select_status
// implementation: src/logic/combat/hit_select.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_hit_select); // "Main"
S(str_cancel_rate); // "Cancel rate"
S(str_conditions_in_hit_select); // "Conditions"
S(str_mode_in_hit_select); // "mode"
S(str_cancel_rate_in_hit_select); // "cancel_rate"
S(str_cancel_rate_air); // "cancel_rate_air"
S(str_pause_dur); // "pause_dur"
S(str_wait_for_damage_dur); // "wait_for_damage_dur"
S(str_trades_delay); // "trades_delay"
S(str_use_server_time); // "use_server_time"
S(str_fake_swing_in_hit_select); // "fake_swing"
S(str_crits_allow_reduce); // "crits_allow_reduce"
S(str_require_damage_in_hit_select); // "require_damage"
S(str_require_sprint_in_hit_select); // "require_sprint"
S(str_require_weapon_in_hit_select); // "require_weapon"
S(str_s_in_hit_select); // "%s"

D(hit_select_cancel_rate_air_setting);                       // setting num "cancel_rate_air"
D(hit_select_pause_dur_setting);                       // setting num "pause_dur"
D(hit_select_wait_for_damage_dur_setting);                       // setting num "wait_for_damage_dur"
D(hit_select_trades_delay_setting);                       // setting num "trades_delay"
D(hit_select_cancel_rate_setting);                       // setting num "cancel_rate"
D(hit_select_require_sprint_setting);                       // setting bool "require_sprint"
D(hit_select_require_weapon_setting);                       // setting bool "require_weapon"
D(hit_select_enum_option_labels_1);                       // enum option labels (const char *[])
D(hit_select_mode_setting);                       // setting enum "mode"
extern "C" unsigned hit_select_mode; // current value of setting "mode" (+0x30)
D(hit_select_use_server_time_setting);                       // setting bool "use_server_time"
D(hit_select_crits_allow_reduce_setting);                       // setting bool "crits_allow_reduce"
D(hit_select_require_damage_setting);                       // setting bool "require_damage"
D(hit_select_fake_swing_setting);                       // setting bool "fake_swing"

extern "C" void hit_select_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_hit_select);
    W_enumw(window, gui, hit_select_mode_setting);
    W_slider(window, gui, hit_select_pause_dur_setting);
    if (hit_select_mode == 0) {
        W_slider(window, gui, hit_select_wait_for_damage_dur_setting);
        W_slider(window, gui, hit_select_trades_delay_setting);
    }
    W_bool(window, gui, hit_select_use_server_time_setting);
    W_bool(window, gui, hit_select_fake_swing_setting);
    if (hit_select_mode == 1) {
        W_bool(window, gui, hit_select_crits_allow_reduce_setting);
        W_bool(window, gui, hit_select_require_damage_setting);
    }
    W_label(gui, window, str_cancel_rate);
    W_slider(window, gui, hit_select_cancel_rate_setting);
    W_slider(window, gui, hit_select_cancel_rate_air_setting);
    W_label(gui, window, str_conditions_in_hit_select);
    W_bool(window, gui, hit_select_require_sprint_setting);
    W_bool(window, gui, hit_select_require_weapon_setting);
}

extern "C" void hit_select_register(void *module) {
    R_enum(module, str_mode_in_hit_select, hit_select_mode_setting);
    R_num(module, str_cancel_rate_in_hit_select, hit_select_cancel_rate_setting);
    R_num(module, str_cancel_rate_air, hit_select_cancel_rate_air_setting);
    R_num(module, str_pause_dur, hit_select_pause_dur_setting);
    R_num(module, str_wait_for_damage_dur, hit_select_wait_for_damage_dur_setting);
    R_num(module, str_trades_delay, hit_select_trades_delay_setting);
    R_bool(module, str_use_server_time, hit_select_use_server_time_setting);
    R_bool(module, str_fake_swing_in_hit_select, hit_select_fake_swing_setting);
    R_bool(module, str_crits_allow_reduce, hit_select_crits_allow_reduce_setting);
    R_bool(module, str_require_damage_in_hit_select, hit_select_require_damage_setting);
    R_bool(module, str_require_sprint_in_hit_select, hit_select_require_sprint_setting);
    R_bool(module, str_require_weapon_in_hit_select, hit_select_require_weapon_setting);
}

extern "C" void hit_select_status(void *self) {
    log_fmt(self, 0x20, str_s_in_hit_select, ((const char **)hit_select_enum_option_labels_1)[hit_select_mode]);

}
