// Auto Clicker — Clicks for you when holding down left-click.
// category: combat   descriptor: 0x2e9330   settings block: 0x2e9528
// functions: gui auto_clicker_gui, register auto_clicker_register, status auto_clicker_status
// implementation: src/logic/combat/auto_clicker.cpp
#pragma once
#include "slinky/api.h"

S(str_in_game); // "In-game"
S(str_inventory_in_auto_clicker); // "Inventory"
S(str_conditions_in_auto_clicker); // "Conditions"
S(str_pattern); // "pattern"
S(str_target_cps_in_auto_clicker); // "target_cps"
S(str_randomize_cps); // "randomize_cps"
S(str_exhaust); // "exhaust"
S(str_allow_block_break); // "allow_block_break"
S(str_allow_inventory); // "allow_inventory"
S(str_inventory_randomize); // "inventory_randomize"
S(str_require_weapon_in_auto_clicker); // "require_weapon"
S(str_require_not_using_item); // "require_not_using_item"
S(str_s_in_auto_clicker); // "%s"

D(auto_clicker_allow_inventory_setting);                       // setting bool "allow_inventory"
D(auto_clicker_allow_inventory);                       // current value of setting "allow_inventory" (+0x18)
D(auto_clicker_inventory_randomize_setting);                       // setting bool "inventory_randomize"
D(auto_clicker_allow_block_break_setting);                       // setting bool "allow_block_break"
D(auto_clicker_require_weapon_setting);                       // setting bool "require_weapon"
D(auto_clicker_require_not_using_item_setting);                       // setting bool "require_not_using_item"
D(auto_clicker_target_cps_setting);                       // setting num "target_cps"
D(auto_clicker_randomize_cps_setting);                       // setting bool "randomize_cps"
D(auto_clicker_enum_option_labels_1);                       // enum option labels (const char *[])
D(auto_clicker_pattern_setting);                       // setting enum "pattern"
extern "C" unsigned auto_clicker_pattern; // current value of setting "pattern" (+0x30)
D(auto_clicker_exhaust_setting);                       // setting bool "exhaust"

extern "C" void auto_clicker_gui(void *gui, void *window) {
    W_label(gui, window, str_in_game);
    W_enumw(window, gui, auto_clicker_pattern_setting);
    W_slider(window, gui, auto_clicker_target_cps_setting);
    W_bool(window, gui, auto_clicker_randomize_cps_setting);
    W_bool(window, gui, auto_clicker_exhaust_setting);
    W_bool(window, gui, auto_clicker_allow_block_break_setting);
    W_label(gui, window, str_inventory_in_auto_clicker);
    W_bool(window, gui, auto_clicker_allow_inventory_setting);
    if (*(unsigned char *)auto_clicker_allow_inventory == 1) {
        W_bool(window, gui, auto_clicker_inventory_randomize_setting);
    }
    W_label(gui, window, str_conditions_in_auto_clicker);
    W_bool(window, gui, auto_clicker_require_weapon_setting);
    W_bool(window, gui, auto_clicker_require_not_using_item_setting);
}

extern "C" void auto_clicker_register(void *module) {
    R_enum(module, str_pattern, auto_clicker_pattern_setting);
    R_num(module, str_target_cps_in_auto_clicker, auto_clicker_target_cps_setting);
    R_bool(module, str_randomize_cps, auto_clicker_randomize_cps_setting);
    R_bool(module, str_exhaust, auto_clicker_exhaust_setting);
    R_bool(module, str_allow_block_break, auto_clicker_allow_block_break_setting);
    R_bool(module, str_allow_inventory, auto_clicker_allow_inventory_setting);
    R_bool(module, str_inventory_randomize, auto_clicker_inventory_randomize_setting);
    R_bool(module, str_require_weapon_in_auto_clicker, auto_clicker_require_weapon_setting);
    R_bool(module, str_require_not_using_item, auto_clicker_require_not_using_item_setting);
}

extern "C" void auto_clicker_status(void *self) {
    log_fmt(self, 0x20, str_s_in_auto_clicker, ((const char **)auto_clicker_enum_option_labels_1)[auto_clicker_pattern]);

}
