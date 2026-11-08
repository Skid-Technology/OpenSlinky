// No Item Release — Hides item release from server.
// category: utility   descriptor: 0x2ec390   settings block: 0x2ec588
// functions: gui no_item_release_gui, register no_item_release_register, status no_item_release_status
// implementation: src/logic/utility/no_item_release.cpp
#pragma once
#include "slinky/api.h"

S(str_main_in_no_item_release); // "Main"
S(str_mode_in_no_item_release); // "mode"
S(str_s_in_no_item_release); // "%s"

D(no_item_release_enum_option_labels_1);                       // enum option labels (const char *[])
D(no_item_release_mode_setting);                       // setting enum "mode"
extern "C" unsigned no_item_release_mode; // current value of setting "mode" (+0x30)

extern "C" void no_item_release_gui(void *gui, void *window) {
    W_label(gui, window, str_main_in_no_item_release);
    W_enumw(window, gui, no_item_release_mode_setting);
}

extern "C" void no_item_release_register(void *module) {
    R_enum(module, str_mode_in_no_item_release, no_item_release_mode_setting);
}

extern "C" void no_item_release_status(void *self) {
    log_fmt(self, 0x20, str_s_in_no_item_release, ((const char **)no_item_release_enum_option_labels_1)[no_item_release_mode]);

}
