// Layout of the client GUI state block (the `gui` argument of every GUI helper).
// Field offsets match the original image; overlapping uses are expressed as unions.
#pragma once
#include <stddef.h>

struct GuiTab { int kind; int pad; const char *label; };     // one entry of the tab bar (kind 6 = unload)
struct GuiVec2 { float x, y; };
struct GuiColor { float r, g, b, a; };
struct GuiRgb { float r, g, b; };
struct GuiHsl { float h, s, l; };

struct GuiCell { float x, y, w, h; };
// one entry of the layout-row stack (see push_layout_row / add_cell)
struct GuiLayoutRow {
    char pad[4]; float top; float width; int padw; float y0; int pad14; unsigned char clip; char pad2[7];
    float bottom; int pad3; long long cols; GuiCell cell[4]; unsigned long long items;
};
static_assert(sizeof(GuiLayoutRow) == 0x78, "layout row size");

struct GuiState {
    GuiLayoutRow rows[5];                  // 0x000  layout-row stack
    long long layout_row_depth;            // 0x258  number of rows currently pushed
    int layout_column_count;               // 0x260
    char pad_264[4];
    unsigned char toast_visible;           // 0x268
    char pad_269;
    char toast_text[0x66];                 // 0x26a  message shown by the toast at the top of the window
    float toast_x;                         // 0x2d0
    float toast_y;                         // 0x2d4
    float toast_fade;                      // 0x2d8  0 -> 1 while the toast fades in
    char pad_2dc[4];
    GuiTab tabs[7];                        // 0x2e0
    unsigned long long selected_tab;       // 0x350
    float tab_highlight_pos;               // 0x358  animated position of the tab highlight
    char pad_35c[4];
    GuiColor accent_color;                 // 0x360
    union {
        int input_mode;                    // 0x370  0 idle, 1 key-bind capture, 2/3 text field being edited
        unsigned input_mode_u;
    };
    char pad_374[4];
    union {                                // 0x378
        char edit_text[0x68];              //   text being edited (input_mode 2/3)
        struct {
            void *keybind_callback;        //   key-bind capture (input_mode 1)
            void *keybind_target;          // 0x380
        };
    };
    void *edit_char_filter;                // 0x3e0  per-character filter of the text field
    void *edit_filter_arg;                 // 0x3e8
    unsigned char edit_confirmed;          // 0x3f0
    char pad_3f1[7];
    union {
        long long popup_kind;              // 0x3f8  0 none, 1 callback, 2 colour picker, 3 confirm, 4 list picker
        int popup_kind_i;
        unsigned popup_kind_u;
    };
    void *popup_context;                   // 0x400
    union {
        void *popup_title;                 // 0x408
        const char *popup_title_text;
        const char ***popup_title_items;
    };
    double popup_open_time;                // 0x410
    float popup_opacity;                   // 0x418
    float popup_height;                    // 0x41c
    unsigned char popup_animating;         // 0x420
    char pad_421[7];
    void *popup_callback;                  // 0x428
    union {                                // 0x430  payload, depends on popup_kind
        char popup_message[0x68];          //   confirm: formatted message
        struct {                           //   colour picker
            GuiRgb popup_rgb;              // 0x430
            GuiHsl popup_hsl;              // 0x43c
        };
        struct {                           //   list picker
            void *popup_list_callback;     // 0x430
            void *popup_list_items;        // 0x438
            union { void *popup_list_extra; unsigned long long popup_list_extra_u; };  // 0x440
            double popup_list_state;       // 0x448
            void *popup_list_view;         // 0x450
            unsigned long long popup_list_count;   // 0x458
        };
    };
    double card_anim_start_time;           // 0x498
    double profile_refresh_time;           // 0x4a0
    GuiVec2 mouse_scene;                   // 0x4a8  mouse position in scaled window space
    GuiVec2 mouse_raw;                     // 0x4b0
    union {
        float card_open_progress;          // 0x4b8
        int card_open_progress_i;
    };
    float scroll;                          // 0x4bc
    float scroll_max;                      // 0x4c0
    float row_height;                      // 0x4c4
    unsigned char click_capture;           // 0x4c8
    unsigned char click_consumed;          // 0x4c9
    unsigned char numeric_drag_enabled;    // 0x4ca
    unsigned char unload_requested;        // 0x4cb
    unsigned char card_layout_mode;        // 0x4cc
    unsigned char wide_layout;             // 0x4cd
};

#define GUI_CHECK(field, off) static_assert(offsetof(GuiState, field) == off, #field)
GUI_CHECK(layout_row_depth, 0x258); GUI_CHECK(toast_visible, 0x268); GUI_CHECK(toast_text, 0x26a);
GUI_CHECK(toast_x, 0x2d0); GUI_CHECK(toast_fade, 0x2d8); GUI_CHECK(tabs, 0x2e0); GUI_CHECK(selected_tab, 0x350);
GUI_CHECK(tab_highlight_pos, 0x358); GUI_CHECK(accent_color, 0x360); GUI_CHECK(input_mode, 0x370);
GUI_CHECK(edit_text, 0x378); GUI_CHECK(keybind_target, 0x380); GUI_CHECK(edit_char_filter, 0x3e0);
GUI_CHECK(edit_filter_arg, 0x3e8); GUI_CHECK(edit_confirmed, 0x3f0); GUI_CHECK(popup_kind, 0x3f8);
GUI_CHECK(popup_context, 0x400); GUI_CHECK(popup_title, 0x408); GUI_CHECK(popup_open_time, 0x410);
GUI_CHECK(popup_opacity, 0x418); GUI_CHECK(popup_height, 0x41c); GUI_CHECK(popup_animating, 0x420);
GUI_CHECK(popup_callback, 0x428); GUI_CHECK(popup_message, 0x430); GUI_CHECK(popup_rgb, 0x430);
GUI_CHECK(popup_hsl, 0x43c); GUI_CHECK(popup_list_extra, 0x440); GUI_CHECK(popup_list_state, 0x448);
GUI_CHECK(popup_list_view, 0x450); GUI_CHECK(popup_list_count, 0x458); GUI_CHECK(card_anim_start_time, 0x498);
GUI_CHECK(profile_refresh_time, 0x4a0); GUI_CHECK(mouse_scene, 0x4a8); GUI_CHECK(mouse_raw, 0x4b0);
GUI_CHECK(card_open_progress, 0x4b8); GUI_CHECK(scroll, 0x4bc); GUI_CHECK(row_height, 0x4c4);
GUI_CHECK(click_capture, 0x4c8); GUI_CHECK(wide_layout, 0x4cd);
