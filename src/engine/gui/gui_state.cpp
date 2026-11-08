// 0x11000: GUI state helpers (drag state at +0x370, flags at +0x4c8..)
#include "slinky/engine.h"
#include "slinky/gui_state.h"
extern "C" __attribute__((noinline)) void mark_click_consumed(GuiState *gui) {
    if (gui->click_capture == 1 && gui->click_consumed == 0) {  gui->click_consumed = 1; }


    SHARED_EPILOG();
}
extern "C" void begin_drag(GuiState *gui, void *object, void *other_object) {
    gui->input_mode = 1;
    gui->keybind_callback = object;
    gui->keybind_target = other_object;
}
extern "C" bool drag_owner_is(GuiState *gui, void *object) {
    if (gui->input_mode != 1) return false;
    return gui->keybind_target == object;
}
extern "C" void end_drag(GuiState *gui) {
    if (gui->input_mode == 2) gui->input_mode = 3;
}
extern "C" void *dragged_payload(GuiState *gui) { return (gui->input_mode_u & ~1u) == 2 ? gui->edit_text : 0; }
extern "C" void request_unload(GuiState *gui) { gui->unload_requested = 1; }
extern "C" void set_tab_animation(GuiState *gui, double t) {
    gui->card_open_progress_i = 0;
    gui->card_anim_start_time = t;
}

extern "C" bool dragged_item_is(GuiState *gui, void *item) {
    if ((gui->input_mode_u & ~1u) != 2) return false;
    return gui->edit_filter_arg == item;
}
extern "C" __attribute__((noinline)) void *take_dragged_item(GuiState *gui) {
    if ((gui->input_mode_u & ~1u) != 2 || gui->edit_confirmed != 1) return 0;
    void *edit_text = gui->edit_text;
    gui->input_mode = 0;
    gui->edit_confirmed = 0;
    return edit_text;
}

struct R16b { long long a, b; };
FN(bool, popup_is_open, (GuiState *gui));
FN(unsigned char, popup_is_animating, (GuiState *gui));
FN(bool, point_in_rect, (R16b r, unsigned long long mouse));
// is the mouse over rect r (and no drag in progress)?
extern "C" __attribute__((noinline)) bool mouse_over_rect_r(GuiState *gui, const R16b *r) {
    if (popup_is_open(gui) && !popup_is_animating(gui)) return false;
    if (!point_in_rect(*r, *(unsigned long long *)&gui->mouse_scene)) return false;
    int st = gui->input_mode;
    return (st == 0) | (st == 3);
}
// clicked inside r this frame (consumes the click)
extern "C" bool clicked_inside_r_frame(GuiState *gui, R16b r) {
    R16b c = r;

    if (popup_is_open(gui) && !popup_is_animating(gui)) return false;
    if (!point_in_rect(c, *(unsigned long long *)&gui->mouse_scene)) return false;
    int st = gui->input_mode;
    if (st != 3 && st != 0) return false;
    if (gui->click_capture != 1 || gui->click_consumed != 0) return false;
    gui->click_consumed = 1;
    return true;
}
// start dragging item b (owner a)
extern "C" void start_dragging_item_b(GuiState *gui, void *item, void *other_object) {
    if ((gui->input_mode_u & ~1u) == 2 && gui->edit_filter_arg == other_object) return;
    gui->input_mode = 2;
    __builtin_memset(gui->edit_text, 0, 0x80);
    gui->edit_char_filter = item;
    gui->edit_filter_arg = other_object;
}

// ---- category tab bar: 7 tabs with a sliding highlight; tab kind 6 = "Unload Slinky?" ----
struct R16t { float x, y, w, h; };
struct V4t { float x, y, z, w; };
struct WT3 { void *ig; double t; };
FN(R16t, add_cell, (GuiState *gui, float h));
FN(V4t, nvg_rgba_f, (float r, float g, float b, float a));
FN(void, nvg_FillColor, (void *nvg, V4t *c));
FN(void, draw_rect_rounded, (WT3 *w, const R16t *r, float round));
FN(float, lerp_clamped_float, (float a, float b, float t));
FN(R16t, make_rect, (float x, float y, float w, float h));
FN(R16t, inset_rect, (R16t r, float d));
FN(void, nvg_FontSize, (void *nvg, float size));
FN(unsigned char, clicked_inside_r_frame_local, (GuiState *gui, R16t r)) __asm__("clicked_inside_r_frame");
FN(float, draw_text, (WT3 *w, R16t r, int align, int col, void *text));
FN(void, confirmation_popup_formatted_message, (void *gui, void *object, void *other_object, const char *fmt, ...));
FN(void, request_unload, (GuiState *gui));
extern "C" const float row_height_42, gui_panel_grey_level, corner_radius_20_in_gui_state, divisor_7, gui_selection_inset, gui_tab_highlight_grey_level, corner_radius_14,
    font_size_16, colour_level_0_866667, colour_level_1_in_gui_state, colour_level_0_278431, colour_level_0_933333, colour_level_0_345098;
extern "C" const double divisor_0_05;
extern "C" const char str_unload_slinky[];   // "Unload Slinky?"
struct Tab { int kind; int padding_04; const char *label; };
extern "C" void draw_tab_bar(GuiState *gui, WT3 *window, float dt) {
    void *nvg = window->ig;
    R16t bar = add_cell(gui, row_height_42);
    float k = gui_panel_grey_level;
    V4t c = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &c);
    draw_rect_rounded(window, &bar, corner_radius_20_in_gui_state);
    float tw = bar.w / divisor_7;
    float pos = lerp_clamped_float(gui->tab_highlight_pos, (float)gui->selected_tab,
                        (float)((double)dt / divisor_0_05));
    gui->tab_highlight_pos = pos;
    float inset = gui_selection_inset;
    R16t sel = inset_rect(make_rect(bar.x + pos * tw, bar.y, tw, bar.h), inset);
    float ks = gui_tab_highlight_grey_level;
    V4t sc = nvg_rgba_f(ks, ks, ks, 1.0f);
    nvg_FillColor(window->ig, &sc);
    draw_rect_rounded(window, &sel, corner_radius_14);
    nvg_FontSize(nvg, font_size_16);
    Tab *tab = (Tab *)(gui->tabs);
    for (unsigned long long i = 0; i != 7; i++, tab++) {
        float cw = bar.w / divisor_7;
        R16t tr = make_rect((float)(long long)i * cw + bar.x, bar.y, cw, bar.h);
        if (clicked_inside_r_frame_local(gui, tr)) {
            if (tab->kind == 6) {
                confirmation_popup_formatted_message(gui, gui, (void *)request_unload, str_unload_slinky);
            } else {
                gui->selected_tab = i;
                gui->input_mode = 0;
            }
        }
        R16t in = inset_rect(tr, inset);
        V4t tc;
        bool hov = mouse_over_rect_r(gui, (const R16b *)&in);
        if (tab->kind == 6) tc = hov ? nvg_rgba_f(colour_level_0_933333, colour_level_0_345098, colour_level_0_345098, 1.0f)
                                     : nvg_rgba_f(colour_level_0_866667, colour_level_0_278431, colour_level_0_278431, 1.0f);
        else tc = hov ? nvg_rgba_f(colour_level_1_in_gui_state, colour_level_1_in_gui_state, colour_level_1_in_gui_state, 1.0f)
                      : nvg_rgba_f(colour_level_0_866667, colour_level_0_866667, colour_level_0_866667, 1.0f);
        nvg_FillColor(window->ig, &tc);
        draw_text(window, in, 1, 0, (void *)tab->label);
    }
}

// ---- click GUI frame: window rect + open animation, scroll, tab bar, category page (module cards or the
// profile list), toast, modal popup ----
struct GWin { void *ig; double t; void *padding_10; float w, h, scale; };
struct R2f { float x, y; };
struct Prof { const char *name, *end; char desc[0x28]; int key; int padding_3c; };
struct GEv { int type, padding_04; union { void *p; int i; } a, b; char rest[0x30]; };
typedef void (*SetBindF)(void *mod, int key);
typedef int v4i11 __attribute__((vector_size(16)));
typedef float v4f11 __attribute__((vector_size(16)));
FN(void, nvg_Save, (void *nvg));
FN(void, nvg_Restore, (void *nvg));
FN(void, nvg_GlobalAlpha, (void *nvg, float a));
FN(void, nvg_Translate, (void *nvg, float x, float y));
FN(void, nvg_Scale, (void *nvg, float x, float y));
FN(R2f, rot_scale, (R2f a, float s));
FN(void, card_open_animation, (GWin *window, float t));
FN(void, reset_layout_columns, (GuiState *gui));
FN(void, push_layout_row, (GuiState *gui, R16t r, float sp, float pad));
FN(void, pop_layout_row, (GuiState *gui, int n));
FN(void, draw_module_cards, (GuiState *gui, GWin *window, void *mgr, unsigned cat, float dt));
FN(int, load_profiles, (void *profiles));
FN(void, dispatch_event, (void *mgr, GEv *e));
FN(void, choose_columns, (GuiState *gui, float w));
FN(Prof *, profile_array, (void *profiles, unsigned long long *n));
FN(bool, rects_intersect, (R16t a, R16t b));
FN(void, begin_content, (GWin *window, GuiState *gui, R16t r, float pad, float f));
FN(float, draw_text_sized, (GWin *window, R16t r, unsigned align, unsigned col, const char *text, const char *end));
FN(SetBindF, profile_set_bind, (void *profiles));
FN(void, draw_module_description, (GuiState *gui, GWin *window, SetBindF cb, void *target, int key, unsigned char *hold, R16t r));
FN(void, ask_sure_before_running_action, (GuiState *gui, void *mgr, void *profiles, const char *name, int kind));
FN(void, draw_texture, (GWin *window, R16t r, unsigned icon, float alpha));
FN(void, set_slider_text, (GuiState *gui, GWin *window, const char *tip, int x, int y));
FN(bool, get_profile_dirty, ());
FN(const char *, get_saved_string, ());
FN(int, libc_strcmp, (const char *left, const char *right));   // strcmp
FN(int, load_profile_name, (const char *name, void *mgr));       // load profile
FN(int, save_profile, (const char *name, bool text, void *mgr));   // save profile
FN(V4t, nvg_trans_rgba_f, (V4t c, float k));
FN(void, file_name_text_filter, (char *buf, char character));
FN(bool, drag_item, (GuiState *gui, GWin *window, R16t r, void (*filter)(char *, char), void *b));
FN(void, draw_tracked_text, (GuiState *gui, GWin *window, const R16t *r));
FN(float, end_layout_row, (GWin *window, GuiState *gui));
FN(float, ease_curve, (float t));
FN(float, nvg_text_bounds, (void *nvg, float x, float y, const char *text, const char *end, const float *bounds));   // nvgTextBounds
FN(void, close_popup_animated, (GuiState *gui, GWin *window));
FN(bool, mouse_over_rect_r_local, (GuiState *gui, R16t r)) __asm__("mouse_over_rect_r");
FN(void, draw_rect_rounded_local, (WT3 *w, R16t r, float round)) __asm__("draw_rect_rounded");
extern "C" const float offset_neg_16, gui_wide_layout_max_width, gui_narrow_layout_max_width, half_in_gui_state, one_in_gui_state_a, row_padding_8,
    row_padding_12, gui_profiles_column_width, colour_level_0_0784314_in_gui_state, row_height_20_in_gui_state_a, font_size_16_in_gui_state, colour_level_0_866667_in_gui_state, rect_extent_5, font_size_14_in_gui_state,
    colour_level_0_6, font_size_15, gui_tab_font_size, offset_10, gui_tab_bar_vertical_offset, colour_level_0_25098_in_gui_state, threshold_600, offset_neg_600;
extern "C" const double divisor_0_3, factor_neg_0_1, offset_2, factor_0_5_in_gui_state, offset_neg_8, divisor_0_1_in_gui_state,
    factor_neg_0_5, offset_neg_5;
extern "C" const v4i11 gui_state_sign_abs_masks, gui_state_sign_abs_masks_second;   // sign / abs masks
extern "C" const char str_default[], str_contains_the_default_configura[], str_default_profile_cannot_be_dele[], str_default_profile_cannot_be_over[], str_profile_name_is_taken[], str_create_new_profile[];
extern "C" const int gui_state_tooltip_strings_as_offsets_from[];   // tooltip strings as offsets from the table ("Click to load", ...)
extern "C" const unsigned gui_state_button_icons[];   // button icons
static inline __attribute__((always_inline)) float mask11(float x, v4i11 m, bool x_or) {
    v4f11 v = {x, 0, 0, 0};
    return ((v4f11)(x_or ? ((v4i11)v ^ m) : ((v4i11)v & m)))[0];
}
static inline __attribute__((always_inline)) void prof_event(void *mgr, int type, GEv *ev) {
    ev->type = type;
    dispatch_event(mgr, ev);
}
FN(void, nvg_Scissor, (void *nvg, float x, float y, float w, float h));
extern "C" void draw_gui_frame(GuiState *gui, GWin *window, void *mgr, void *profiles) {
    void *nvg = window->ig;
    nvg_Save(nvg);
    card_open_animation(window, gui->card_open_progress);
    float ww = window->w;
    float pw = offset_neg_16 + ww;
    float lim = gui->wide_layout ? gui_wide_layout_max_width : gui_narrow_layout_max_width;
    pw = pw < lim ? pw : lim;
    float px = (ww - pw) * half_in_gui_state;
    float *scroll = (float *)(&gui->scroll);
    float s = *scroll;
    if (0.0f > s) *scroll = 0.0f;
    else if (s > gui->scroll_max) *scroll = gui->scroll_max;
    float one = one_in_gui_state_a;
    R2f m = rot_scale(*(R2f *)&gui->mouse_raw, one / window->scale);
    *(R2f *)&gui->mouse_scene = m;
    gui->mouse_scene.y = m.y + *scroll;
    nvg_Translate(window->ig, 0.0f, mask11(*scroll, gui_state_sign_abs_masks, true));
    reset_layout_columns(gui);
    R16t win = make_rect(px, 0.0f, pw, window->h);
    float dt = (float)(window->t - gui->card_anim_start_time);
    double dtd = dt;
    float op = (float)((double)gui->card_open_progress + dtd / divisor_0_3);
    gui->card_open_progress = one < op ? one : op;
    push_layout_row(gui, win, row_padding_8, row_padding_12);
    pop_layout_row(gui, 1);
    draw_tab_bar(gui, (WT3 *)window, dt);
    unsigned long long cat = gui->selected_tab;
    float off = (float)cat - gui->tab_highlight_pos;
    if (off != 0.0f) {   // page slides while the tab highlight moves
        void *sig = window->ig;
        float d = mask11(off, gui_state_sign_abs_masks_second, false);
        nvg_GlobalAlpha(sig, half_in_gui_state < d ? 0.0f : one - (d + d));
        nvg_Translate(sig, (float)((double)off * ((double)window->w * factor_neg_0_1)), 0.0f);
        cat = gui->selected_tab;
    }
    if (cat >= 7) cat = 0;   // tabs[7] would run into selected_tab
    switch ((unsigned)gui->tabs[cat].kind) {
    case 0: draw_module_cards(gui, window, mgr, 0, dt); break;
    case 4: draw_module_cards(gui, window, mgr, 4, dt); break;
    case 2: draw_module_cards(gui, window, mgr, 2, dt); break;
    case 3: draw_module_cards(gui, window, mgr, 3, dt); break;
    case 1: draw_module_cards(gui, window, mgr, 1, dt); break;
    case 5: {   // profiles
        if (gui->input_mode == 0 && window->t > gui->profile_refresh_time + offset_2) {
            int e = load_profiles(profiles);
            if (e) {
                GEv ev = {};
                ev.a.i = e;
                prof_event(mgr, 0xc, &ev);
            }
            gui->profile_refresh_time = window->t;
        }
        void *pig = window->ig;
        choose_columns(gui, gui_profiles_column_width);
        unsigned long long n;
        Prof *list = profile_array(profiles, &n);
        for (unsigned long long i = 0; i <= n; i++) {
            R16t row = add_cell(gui, gui->row_height);
            if (!rects_intersect(make_rect(0.0f, gui->scroll, window->w, window->h), row)) continue;
            Prof *ent = i ? &list[i - 1] : 0;
            V4t bg = nvg_rgba_f(colour_level_0_0784314_in_gui_state, colour_level_0_0784314_in_gui_state, colour_level_0_0784314_in_gui_state, 1.0f);
            nvg_FillColor(pig, &bg);
            draw_rect_rounded_local((WT3 *)window, row, row_height_20_in_gui_state_a);
            begin_content(window, gui, row, row_padding_8, 12.0f);
            pop_layout_row(gui, 1);
            void *tig = window->ig;
            R16t t = add_cell(gui, row_height_20_in_gui_state_a);
            nvg_FontSize(tig, font_size_16_in_gui_state);
            V4t tc = nvg_rgba_f(colour_level_0_866667_in_gui_state, colour_level_0_866667_in_gui_state, colour_level_0_866667_in_gui_state, 1.0f);
            nvg_FillColor(tig, &tc);
            if (ent) {
                float tw = draw_text_sized(window, t, 0, 0, ent->name, ent->end);
                R16t br = make_rect(tw + rect_extent_5, t.y, t.w - tw + t.x, t.h);
                draw_module_description(gui, window, profile_set_bind(profiles), ent, ent->key, 0, br);
            } else {
                draw_text_sized(window, t, 0, 0, str_default /* "Default" */, 0);
            }
            for (unsigned j = 0; j != 4; j++) {   // load, save, open folder, delete
                R16t b = make_rect(t.x + t.w - (float)(int)(20 * (4 - j)) - (float)(int)(12 * (3 - j)), t.y,
                                 row_height_20_in_gui_state_a, 20.0f);
                float alpha = half_in_gui_state;
                if (ent || !j) {
                    alpha = one;
                    if (mouse_over_rect_r_local(gui, b)) {
                        set_slider_text(gui, window, (const char *)gui_state_tooltip_strings_as_offsets_from + gui_state_tooltip_strings_as_offsets_from[j],
                                 (int)((double)b.x + (double)b.w * factor_0_5_in_gui_state), (int)b.y);
                        if (clicked_inside_r_frame_local(gui, b)) {
                            const char *name = ent ? ent->name : 0;
                            int error_code;
                            const char *msg;
                            switch (j) {
                            case 0:
                                if (get_profile_dirty()) {
                                    ask_sure_before_running_action(gui, mgr, profiles, name, 0);
                                    goto icon;
                                }
                                error_code = load_profile_name(name, mgr);
                                goto result;
                            case 1:
                                if (!ent) {
                                    error_code = 0x1773;
                                    msg = str_default_profile_cannot_be_over;   // "Default profile cannot be overwritten"
                                    goto error;
                                }
                                if (get_saved_string() && libc_strcmp(name, get_saved_string()) == 0) {
                                    error_code = save_profile(name, true, mgr);
                                    goto result;
                                }
                                ask_sure_before_running_action(gui, mgr, profiles, name, 1);
                                goto icon;
                            case 2:
                                ask_sure_before_running_action(gui, mgr, profiles, name, 2);
                                goto icon;
                            case 3:
                                if (ent) {
                                    ask_sure_before_running_action(gui, mgr, profiles, name, 3);
                                    goto icon;
                                }
                                error_code = 0x1774;
                                msg = str_default_profile_cannot_be_dele;   // "Default profile cannot be deleted"
                                goto error;
                            }
                        result:
                            if (!error_code) {
                                GEv ev = {};
                                ev.a.p = (void *)name;
                                ev.b.i = (int)j;
                                prof_event(mgr, 0xb, &ev);
                                gui->profile_refresh_time = -1.0;
                                goto icon;
                            }
                            msg = error_code == 0x1774 ? str_default_profile_cannot_be_dele : error_code == 0x1773 ? str_default_profile_cannot_be_over : 0;
                        error : {
                            GEv ev = {};
                            ev.a.i = error_code;
                            ev.b.p = (void *)msg;
                            prof_event(mgr, 0xc, &ev);
                        }
                        }
                    }
                }
            icon:
                draw_texture(window, b, gui_state_button_icons[j], alpha);
            }
            if (!gui->card_layout_mode) {   // description line
                R16t d = add_cell(gui, row_height_20_in_gui_state_a);
                nvg_FontSize(tig, font_size_14_in_gui_state);
                V4t dc = nvg_rgba_f(colour_level_0_6, colour_level_0_6, colour_level_0_6, 1.0f);
                nvg_FillColor(tig, &dc);
                draw_text((WT3 *)window, d, 0, 0, ent ? (void *)ent->desc : (void *)str_contains_the_default_configura);
            }
            gui->row_height = end_layout_row(window, gui);
        }
        // "CREATE NEW PROFILE": a text field once clicked
        R16t cr = add_cell(gui, gui->row_height);
        void *cig = window->ig;
        V4t c0 = nvg_rgba_f(colour_level_0_0784314_in_gui_state, colour_level_0_0784314_in_gui_state, colour_level_0_0784314_in_gui_state, 1.0f);
        V4t c1 = nvg_trans_rgba_f(c0, half_in_gui_state);
        nvg_FillColor(cig, &c1);
        draw_rect_rounded_local((WT3 *)window, cr, row_height_20_in_gui_state_a);
        if (drag_item(gui, window, cr, file_name_text_filter, 0)) {
            nvg_FontSize(cig, font_size_16_in_gui_state);
            V4t wc = nvg_rgba_f(one, one, one, 1.0f);
            nvg_FillColor(cig, &wc);
            R16t ir = {cr.x, (float)((double)cr.h * factor_0_5_in_gui_state + (double)cr.y + offset_neg_8), cr.w, gui_tab_font_size};
            draw_tracked_text(gui, window, &ir);
            const char *nm = (const char *)take_dragged_item(gui);
            if (nm) {
                int r = save_profile(nm, false, mgr);
                GEv ev = {};
                if (!r) {
                    ev.a.p = (void *)nm;
                    ev.b.i = 1;
                    prof_event(mgr, 0xb, &ev);
                    gui->profile_refresh_time = -1.0;
                } else {
                    ev.a.i = r;
                    ev.b.p = r == 0x3f4 ? (void *)str_profile_name_is_taken : 0;   // "Profile name is taken."
                    prof_event(mgr, 0xc, &ev);
                }
            }
        } else {
            nvg_FontSize(cig, font_size_15);
            float k = mouse_over_rect_r_local(gui, cr) ? one : colour_level_0_866667_in_gui_state;
            V4t tc2 = nvg_rgba_f(k, k, k, 1.0f);
            nvg_FillColor(cig, &tc2);
            draw_text((WT3 *)window, cr, 1, 1, (void *)str_create_new_profile);   // "CREATE NEW PROFILE"
        }
        break;
    }
    }
    end_layout_row(window, gui);
    if (gui->toast_visible) {   // toast at the top, fades in
        gui->toast_fade = (float)((double)gui->toast_fade + dtd / divisor_0_1_in_gui_state);
        void *tig = window->ig;
        nvg_Save(tig);
        nvg_Translate(tig, gui->toast_x, gui->toast_y);
        if (gui->toast_fade > one) {
            gui->toast_fade = 1.0f;
        } else {
            float e = ease_curve(gui->toast_fade);
            nvg_GlobalAlpha(tig, e);
            nvg_Scale(tig, e, e);
        }
        nvg_FontSize(tig, font_size_14_in_gui_state);
        float tw = nvg_text_bounds(tig, 0.0f, 0.0f, gui->toast_text, 0, (const float *)0);
        R16t tr = make_rect((float)((double)tw * factor_neg_0_5 + offset_neg_5), gui_tab_bar_vertical_offset, tw + offset_10, 20.0f);
        V4t bc = nvg_rgba_f(colour_level_0_25098_in_gui_state, colour_level_0_25098_in_gui_state, colour_level_0_25098_in_gui_state, 1.0f);
        nvg_FillColor(tig, &bc);
        draw_rect_rounded_local((WT3 *)window, tr, rect_extent_5);
        V4t fc = nvg_rgba_f(colour_level_0_866667_in_gui_state, colour_level_0_866667_in_gui_state, colour_level_0_866667_in_gui_state, 1.0f);
        nvg_FillColor(tig, &fc);
        draw_text((WT3 *)window, tr, 1, 0, gui->toast_text);
        nvg_Restore(tig);
        gui->toast_visible = 0;
    } else {
        gui->toast_fade = 0.0f;
    }
    nvg_Restore(nvg);
    float mx = (float)gui->layout_column_count - window->h + row_padding_12;
    gui->scroll_max = 0.0f > mx ? 0.0f : mx;
    if (popup_is_open(gui)) {   // modal popup over everything (unscrolled)
        nvg_Save(nvg);
        gui->mouse_scene.y -= gui->scroll;
        card_open_animation(window, gui->popup_opacity);
        reset_layout_columns(gui);
        R16t pr = win;
        if (pr.w > threshold_600) {
            pr.x = (float)((double)(pr.w + offset_neg_600) * factor_0_5_in_gui_state + (double)pr.x);
            pr.w = threshold_600;
        }
        push_layout_row(gui, pr, row_padding_8, row_padding_12);
        close_popup_animated(gui, window);
        end_layout_row(window, gui);
        nvg_Restore(nvg);
    }
    mark_click_consumed(gui);
    gui->click_consumed = gui->click_capture;
    gui->card_anim_start_time = window->t;
}
