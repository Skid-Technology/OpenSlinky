// GUI - the client's settings window (ClickGUI).
//
// The window itself is drawn by the engine (draw_gui_frame). These callbacks open and close
// it, pass keyboard / mouse input to it, free the mouse while it is open, and save the
// settings when it closes.
//
// Functions:
//   gui_get_scale            the window scale ("scale" setting)
//   gui_handle_input_event   every key / mouse event: 1 = the GUI used it
//   gui_on_tick              every tick: keep the mouse free
//   gui_on_toggle            opened / closed
//   gui_on_hud_tick          HUD hook: ask for redraws
//   gui_on_hud_render        every frame: draw the window
//   gui_on_shutdown          the client is unloading
#include "slinky/engine.h"

struct V4 { float x, y, z, w; };   // a colour (r, g, b, a)

struct InputEvt {
    int type;               // +0x00 2 = keyboard
    int code;               // +0x04 key code
    int padding_08;
    unsigned char down;     // +0x0c 1 = pressed
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor / draw event
    unsigned char on;    // +0x18 1 = switched on
};

// A notification for the GUI.
struct Notice { int kind; int padding_04; int value; char rest[0x3c]; };
enum { NOTICE_SAVE_FAILED = 0xc };

// ---- settings ----
extern "C" unsigned gui_scale_value;               // "scale" (index into the table)
extern "C" const float gui_scale_table[];
extern "C" unsigned char gui_allow_input_value;    // "allow_input": let typing keys reach the game
extern "C" unsigned char gui_compact_value;        // "compact"
extern "C" unsigned char gui_wide_value;           // "wide"
extern "C" float gui_accent_color_setting_field_0x8, gui_accent_color_setting_field_0xc, gui_accent_color_setting_field_0x10;   // "accent_color" (r, g, b)

// ---- module state ----
extern "C" char gui_module_descriptor[];
extern "C" unsigned char gui_gui_open;
extern "C" int gui_toggle_key;                     // the GUI's keybind
extern "C" char gui_state_block[];                 // the window's state
extern "C" V4 gui_accent_colour;
extern "C" unsigned char gui_compact_mode;
extern "C" unsigned char gui_wide_mode;
extern "C" unsigned char gui_shutting_down;

// ---- engine functions ----
FN(unsigned char, handle_gui_input, (void *state, void *evt));
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(unsigned char, java_call_static_mouse_is_grabbed, (void *jvm));
FN(void, java_call_static_mouse_set_grabbed, (void *jvm, int x));
FN(int, java_get_static_display_width, (void *jvm));
FN(int, java_get_static_display_height, (void *jvm));
FN(void, java_call_static_mouse_set_cursor_position, (void *jvm, int x, int y));
FN(double, time_now, ());
FN(void, set_tab_animation, (void *state, double v));
FN(unsigned char, is_in_game, (void *jvm, void *object));
FN(void, close_popup_now, (void *state));
FN(void, reset_drag_state, (void *state));
FN(int, save_settings, (void *module_manager, void *profile_list));   // 0 = ok, else an error code
FN(void, dispatch_event, (void *jvm, Notice *n));
FN(V4, nvg_rgb_f, (float a, float b, float c));
FN(void, draw_gui_frame, (void *state, void *event, void *other_object, void *y));

enum { KEY_EVENT = 2, KEY_F11 = 0x57 };

extern "C" float gui_get_scale() {
    return gui_scale_table[gui_scale_value];
}

// Returns 1 when the GUI took the event (the game does not see it).
extern "C" unsigned char gui_handle_input_event(InputEvt *evt, unsigned char shift, void *module_manager) {
    if (gui_gui_open == 1 && handle_gui_input(gui_state_block, evt))
        return 1;

    int type = evt->type;
    if (type == KEY_EVENT && evt->down == 1 && evt->code != 0) {
        unsigned char open = gui_gui_open;
        int code = evt->code;
        // The keybind opens / closes it; F11 closes it.
        if (!shift && code == gui_toggle_key) {
            queue_module_toggle(module_manager, gui_module_descriptor, open ^ 1);
            return 1;
        }
        if (code == KEY_F11 && open) {
            queue_module_toggle(module_manager, gui_module_descriptor, 0);
            return 1;
        }
    }

    // While open: keep everything from the game, except (with "allow_input") keys above 5.
    if (gui_gui_open != 1)
        return 0;
    if (type != KEY_EVENT)
        return 1;
    if (!gui_allow_input_value)
        return 1;
    if ((unsigned)evt->code <= 5)
        return 1;
    return 0;
}

// While open: never let the game grab the mouse.
extern "C" void gui_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (java_call_static_mouse_is_grabbed(ctx->jvm))
        java_call_static_mouse_set_grabbed(ctx->jvm, 0);
}

extern "C" void gui_on_toggle(EvHook *hook) {
    if (hook->ev != gui_module_descriptor)
        return;
    Ctx *ctx = hook->ctx;
    void *jvm = ctx->jvm;

    if (hook->on == 1) {
        // Opened: free the mouse in the middle of the screen.
        if (java_call_static_mouse_is_grabbed(jvm)) {
            int width = java_get_static_display_width(ctx->jvm);
            int height = java_get_static_display_height(ctx->jvm);
            java_call_static_mouse_set_cursor_position(ctx->jvm, width / 2, height / 2);
            java_call_static_mouse_set_grabbed(ctx->jvm, 0);
        }
        set_tab_animation(gui_state_block, time_now());
        return;
    }

    // Closed: give the mouse back to the game and save the settings.
    if (is_in_game(jvm, ctx->minecraft))
        java_call_static_mouse_set_grabbed(ctx->jvm, 1);
    close_popup_now(gui_state_block);
    reset_drag_state(gui_state_block);
    if (gui_shutting_down)
        return;
    int error = save_settings(ctx->module_manager, ctx->profile_list);
    if (!error)
        return;
    Notice notice = {};
    notice.kind = NOTICE_SAVE_FAILED;
    notice.value = error;
    dispatch_event(ctx->module_manager, &notice);
}

extern "C" void gui_on_hud_tick(unsigned char *h) {
    h[0x10] |= 3;
}

extern "C" void gui_on_hud_render(EvHook *hook) {
    Ctx *ctx = hook->ctx;
    gui_accent_colour = nvg_rgb_f(gui_accent_color_setting_field_0x8, gui_accent_color_setting_field_0xc, gui_accent_color_setting_field_0x10);
    gui_compact_mode = gui_compact_value;
    gui_wide_mode = gui_wide_value;
    draw_gui_frame(gui_state_block, hook->ev, ctx->module_manager, ctx->profile_list);
}

extern "C" void gui_on_shutdown(Hook *hook) {
    gui_shutting_down = 1;
}
