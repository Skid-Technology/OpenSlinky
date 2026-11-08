// Knockback Displace — Allows dealing knockback at a different angle.
// category: combat   descriptor: 0x2f5d60   settings block: 0x2f5f58
// functions: gui knockback_displace_gui, register knockback_displace_register, status knockback_displace_status
// implementation: src/logic/combat/knockback_displace.cpp
#pragma once
#include "slinky/api.h"

S(str_0f_in_knockback_displace); // "%.0f°"
S(str_main_in_knockback_displace); // "Main"
S(str_condition); // "Condition"
S(str_direction_in_knockback_displace); // "direction"
S(str_use_last_strafe); // "use_last_strafe"
S(str_angle_in_knockback_displace); // "angle"
S(str_cooldown_in_knockback_displace); // "cooldown"
S(str_hide_rotation); // "hide_rotation"
S(str_require_no_kb); // "require_no_kb"
S(str_require_weapon_in_knockback_displace); // "require_weapon"
S(str_require_kb_enchant); // "require_kb_enchant"
S(str_require_facing_away); // "require_facing_away"

D(knockback_displace_cooldown_setting);                       // setting num "cooldown"
D(knockback_displace_angle_setting);                       // setting num "angle"
extern "C" float knockback_displace_angle;    // current value of setting "angle" (+0x30)
D(knockback_displace_require_no_kb_setting);                       // setting bool "require_no_kb"
D(knockback_displace_require_weapon_setting);                       // setting bool "require_weapon"
D(knockback_displace_require_kb_enchant_setting);                       // setting bool "require_kb_enchant"
D(knockback_displace_require_facing_away_setting);                       // setting bool "require_facing_away"
D(knockback_displace_direction_setting);                       // setting enum "direction"
extern "C" unsigned knockback_displace_direction_value; // current value of setting "direction" (+0x30)
D(knockback_displace_hide_rotation_setting);                       // setting bool "hide_rotation"
D(knockback_displace_use_last_strafe_setting);                       // setting bool "use_last_strafe"

extern "C" void knockback_displace_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_knockback_displace);
    W_enumw(window, gui, knockback_displace_direction_setting);
    switch (knockback_displace_direction_value) {
    case 3:
    case 4:
        W_bool(window, gui, knockback_displace_use_last_strafe_setting);
        break;
    }
    W_slider(window, gui, knockback_displace_cooldown_setting);
    W_slider(window, gui, knockback_displace_angle_setting);
    W_bool(window, gui, knockback_displace_hide_rotation_setting);
    W_label(gui, window, str_condition);
    W_bool(window, gui, knockback_displace_require_no_kb_setting);
    W_bool(window, gui, knockback_displace_require_weapon_setting);
    W_bool(window, gui, knockback_displace_require_kb_enchant_setting);
    W_bool(window, gui, knockback_displace_require_facing_away_setting);
}

extern "C" void knockback_displace_register(void *module) {
    R_enum(module, str_direction_in_knockback_displace, knockback_displace_direction_setting);
    R_bool(module, str_use_last_strafe, knockback_displace_use_last_strafe_setting);
    R_num(module, str_angle_in_knockback_displace, knockback_displace_angle_setting);
    R_num(module, str_cooldown_in_knockback_displace, knockback_displace_cooldown_setting);
    R_bool(module, str_hide_rotation, knockback_displace_hide_rotation_setting);
    R_bool(module, str_require_no_kb, knockback_displace_require_no_kb_setting);
    R_bool(module, str_require_weapon_in_knockback_displace, knockback_displace_require_weapon_setting);
    R_bool(module, str_require_kb_enchant, knockback_displace_require_kb_enchant_setting);
    R_bool(module, str_require_facing_away, knockback_displace_require_facing_away_setting);
}

extern "C" void knockback_displace_status(void *self) {
    log_fmt(self, 0x20, str_0f_in_knockback_displace, (double)knockback_displace_angle);

}
