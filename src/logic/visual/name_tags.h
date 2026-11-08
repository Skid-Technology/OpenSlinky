// Name Tags — Makes player name tags easier to see.
// category: visual   descriptor: 0x2f02d0   settings block: 0x2f04c8
// functions: gui name_tags_gui, register name_tags_register
// implementation: src/logic/visual/name_tags.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_name_tags); // "Main"
S(str_extra_info); // "Extra info"
S(str_range_in_name_tags); // "range"
S(str_scale_in_name_tags); // "scale"
S(str_show_own_name_tag_in_name_tags); // "show_own_name_tag"
S(str_use_display_name); // "use_display_name"
S(str_distance_scaling_in_name_tags); // "distance_scaling"
S(str_background); // "background"
S(str_text_shadow_in_name_tags); // "text_shadow"
S(str_show_health); // "show_health"
S(str_health_fmt); // "health_fmt"
S(str_show_distance); // "show_distance"
S(str_show_equipment); // "show_equipment"
S(str_show_enchants); // "show_enchants"

D(name_tags_scale_setting); // setting num "scale"
D(name_tags_distance_scaling_setting); // setting bool "distance_scaling"
D(name_tags_background_setting); // setting bool "background"
D(name_tags_text_shadow_setting); // setting bool "text_shadow"
D(name_tags_show_equipment_setting); // setting bool "show_equipment"
D(name_tags_show_equipment); // current value of setting "show_equipment" (+0x18)
D(name_tags_show_enchants_setting); // setting bool "show_enchants"
D(name_tags_range_setting); // setting num "range"
D(name_tags_show_own_name_tag_setting); // setting bool "show_own_name_tag"
D(name_tags_use_display_name_setting); // setting bool "use_display_name"
D(name_tags_show_health_setting); // setting bool "show_health"
D(name_tags_show_health); // current value of setting "show_health" (+0x18)
D(name_tags_show_distance_setting); // setting bool "show_distance"
D(name_tags_health_fmt_setting); // setting enum "health_fmt"

extern "C" void name_tags_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_name_tags);
    W_slider(window, gui, name_tags_range_setting);
    W_slider(window, gui, name_tags_scale_setting);
    W_bool(window, gui, name_tags_show_own_name_tag_setting);
    W_bool(window, gui, name_tags_use_display_name_setting);
    W_bool(window, gui, name_tags_distance_scaling_setting);
    W_bool(window, gui, name_tags_background_setting);
    W_bool(window, gui, name_tags_text_shadow_setting);
    W_label(gui, window, str_extra_info);
    W_bool(window, gui, name_tags_show_health_setting);
    if (*(unsigned char *)name_tags_show_health == 1) {
        W_enumw(window, gui, name_tags_health_fmt_setting);
    }
    W_bool(window, gui, name_tags_show_distance_setting);
    W_bool(window, gui, name_tags_show_equipment_setting);
    if (*(unsigned char *)name_tags_show_equipment == 1) {
        W_bool(window, gui, name_tags_show_enchants_setting);
    } else {
        EPILOG_NOP();
    }
}

extern "C" void name_tags_register(void *module) {
    R_num(module, str_range_in_name_tags, name_tags_range_setting);
    R_num(module, str_scale_in_name_tags, name_tags_scale_setting);
    R_bool(module, str_show_own_name_tag_in_name_tags, name_tags_show_own_name_tag_setting);
    R_bool(module, str_use_display_name, name_tags_use_display_name_setting);
    R_bool(module, str_distance_scaling_in_name_tags, name_tags_distance_scaling_setting);
    R_bool(module, str_background, name_tags_background_setting);
    R_bool(module, str_text_shadow_in_name_tags, name_tags_text_shadow_setting);
    R_bool(module, str_show_health, name_tags_show_health_setting);
    R_enum(module, str_health_fmt, name_tags_health_fmt_setting);
    R_bool(module, str_show_distance, name_tags_show_distance_setting);
    R_bool(module, str_show_equipment, name_tags_show_equipment_setting);
    R_bool(module, str_show_enchants, name_tags_show_enchants_setting);
}
