// Notifications — Informs you when certain actions are performed.
// category: visual   descriptor: 0x2ed320   settings block: 0x2ed518
// functions: gui notifications_gui, register notifications_register
// implementation: src/logic/visual/notifications.cpp
#pragma once
#include "slinky/api.h"

S(str_hide_if); // "Hide if"
S(str_visible_mod_categories); // "Visible mod categories"
S(str_hide_ingame); // "hide_ingame"
S(str_hide_hold_notifs); // "hide_hold_notifs"
S(str_hide_bind_only); // "hide_bind_only"
S(str_show_combat_in_notifications); // "show_combat"
S(str_show_move_in_notifications); // "show_move"
S(str_show_block_in_notifications); // "show_block"
S(str_show_visual_in_notifications); // "show_visual"
S(str_show_utility_in_notifications); // "show_utility"

D(notifications_hide_bind_only_setting); // setting bool "hide_bind_only"
D(notifications_hide_hold_notifs_setting); // setting bool "hide_hold_notifs"
D(notifications_show_combat_setting); // setting bool "show_combat"
D(notifications_show_move_setting); // setting bool "show_move"
D(notifications_show_block_setting); // setting bool "show_block"
D(notifications_show_visual_setting); // setting bool "show_visual"
D(notifications_show_utility_setting); // setting bool "show_utility"
D(notifications_hide_ingame_setting); // setting bool "hide_ingame"

extern "C" void notifications_gui(void *gui, void *window) {
    W_label(gui, window, str_hide_if);
    W_bool(window, gui, notifications_hide_ingame_setting);
    W_bool(window, gui, notifications_hide_hold_notifs_setting);
    W_bool(window, gui, notifications_hide_bind_only_setting);
    W_label(gui, window, str_visible_mod_categories);
    W_bool(window, gui, notifications_show_combat_setting);
    W_bool(window, gui, notifications_show_move_setting);
    W_bool(window, gui, notifications_show_block_setting);
    W_bool(window, gui, notifications_show_visual_setting);
    W_bool(window, gui, notifications_show_utility_setting);
}

extern "C" void notifications_register(void *module) {
    R_bool(module, str_hide_ingame, notifications_hide_ingame_setting);
    R_bool(module, str_hide_hold_notifs, notifications_hide_hold_notifs_setting);
    R_bool(module, str_hide_bind_only, notifications_hide_bind_only_setting);
    R_bool(module, str_show_combat_in_notifications, notifications_show_combat_setting);
    R_bool(module, str_show_move_in_notifications, notifications_show_move_setting);
    R_bool(module, str_show_block_in_notifications, notifications_show_block_setting);
    R_bool(module, str_show_visual_in_notifications, notifications_show_visual_setting);
    R_bool(module, str_show_utility_in_notifications, notifications_show_utility_setting);
}
