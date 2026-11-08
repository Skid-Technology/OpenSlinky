// GUI: text rendering helpers - labels, wrapped descriptions, numeric filters.
#include "slinky/engine.h"
#include "slinky/gui_state.h"

FN(unsigned long long, libc_strlen, (const char *str));
// file-name text filter: printable, no slashes, max 15 chars
extern "C" void file_name_text_filter(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    unsigned char lng = n >= 0xf;

    unsigned char np = (unsigned char)(character - 0x20) >= 0x5f;

    if (np | lng) return;
    if (character == 0x2f || character == 0x5c) return;
    buf[n] = character;
}

struct R16c { float x, y, w, h; };
struct V4 { float x, y, z, w; };
FN(R16c, add_cell, (const void *src, float f));
FN(V4, nvg_rgba_f, (float r, float g, float b, float a));
FN(void, nvg_FillColor, (void *nvg, V4 *c));
FN(void, nvg_FontSize, (void *nvg, float f));
FN(float, draw_text, (void *window, R16c r, int a, int b, void *x));
extern "C" const float row_height_19, gui_section_label_top_margin, gui_section_label_grey_level, font_size_15_in_gui_text_a;
extern "C" float draw_section_label(const void *src, void **window, void *other_object) {
    void *nvg = window[0];
    R16c r = add_cell(src, row_height_19);
    r.y += gui_section_label_top_margin;
    float g = gui_section_label_grey_level;
    V4 c = nvg_rgba_f(g, g, g, 1.0f);
    nvg_FillColor(nvg, &c);
    nvg_FontSize(nvg, font_size_15_in_gui_text_a);
    return draw_text(window, r, 0, 0, other_object);
}

FN(void *, libc_malloc, (unsigned long long n));
FN(void, libc_free, (void *ptr));
FN(void *, libc_memcpy, (void *dest, const void *src, unsigned long long n));
FN(void, confirmation_popup_formatted_message, (void *gui, void *object, void *other_object, const char *fmt, ...));
FN(void, run_confirmed_profile_action, (void *p));
extern "C" const int format_string_offsets[];
extern "C" const char str_default[];   // "Default"
struct Confirm { GuiState *gui; void *module_manager; void *profile_list; char *name; int kind; };
// ask "Are you sure?" before running action `kind` on profile `name`
extern "C" void ask_sure_before_running_action(GuiState *gui, void *module_manager, void *profile_list, const char *name, int kind) {
    Confirm *c = (Confirm *)libc_malloc(0x28);
    if (!c) return;
    c->gui = gui; c->module_manager = module_manager; c->profile_list = profile_list; c->name = 0; c->kind = kind;
    if (name) {
        unsigned long long n = libc_strlen(name);
        char *s = (char *)libc_malloc(n + 1);
        if (!s) { libc_free(c); return; }
        libc_memcpy(s, name, n);
        s[n] = 0;
        c->name = s;
    } else {
        name = str_default;
    }
    const char *fmt = (const char *)format_string_offsets + format_string_offsets[(unsigned)kind];
    confirmation_popup_formatted_message(gui, c, (void *)run_confirmed_profile_action, fmt, name);
}

FN(int, load_profile_name, (const char *name, void *mods));
FN(int, save_profile, (const char *name, bool text, void *mods));
FN(int, open_profiles_folder, ());
FN(int, delete_profile_file, (const char *name));
FN(void, dispatch_event, (void *mgr, void *object));
struct ProfEvent { int type; int padding_04; union { int code; const char *name; }; int kind; char rest[0x2c]; };
// run the confirmed profile action (0 load, 1 save, 2 open folder, 3 delete) and report it
extern "C" void run_confirmed_profile_action(void *cv) {
    Confirm *c = (Confirm *)cv;
    int error_code;
    switch ((unsigned)c->kind) {
    case 0: error_code = load_profile_name(c->name, c->module_manager); break;
    case 1: error_code = save_profile(c->name, true, c->module_manager); break;
    case 2: error_code = open_profiles_folder(); break;
    case 3: error_code = delete_profile_file(c->name); break;
    default: error_code = 0; goto ok;
    }
    if (error_code) {
        ProfEvent e = {};
        e.type = 0xc;
        e.code = error_code;
        dispatch_event(c->module_manager, &e);
        goto done;
    }
ok:
    {
        ProfEvent e = {};
        e.type = 0xb;
        e.name = c->name;
        e.kind = c->kind;
        dispatch_event(c->module_manager, &e);
    }
done:
    c->gui->profile_refresh_time = -1.0;
    libc_free(c->name);
    libc_free(c);
}

// ---- animated toggle switch; returns true when clicked ----
struct Anim { float pos; int padding_04; double last; };
struct WT2 { void *ig; double t; };
FN(float, ease_curve, (float t));   // smoothstep
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(bool, mouse_over_rect_r, (GuiState *gui, const R16c *r));
FN(unsigned char, clicked_inside_r_frame, (GuiState *gui, R16c r));
FN(V4, nvg_lerp_rgba, (V4 a, V4 b, float t));
FN(void, draw_rect_rounded, (WT2 *window, const R16c *r, float round));
FN(R16c, inset_rect, (R16c r, float d));
FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_Circle, (void *nvg, float x, float y, float rad));
FN(void, nvg_Fill, (void *nvg));
extern "C" const double tenth_in_gui_text, divisor_neg_0_1_in_gui_text;
extern "C" const float threshold_1_in_gui_text_b, gui_toggle_idle_grey_level, gui_toggle_hover_grey_level, corner_radius_10_in_gui_text, inset_6_in_gui_text_a, gui_text_background_grey_level, factor_0_5_in_gui_text_a;
extern "C" bool draw_animated_description(GuiState *gui, WT2 *window, R16c *r, Anim *an, unsigned char *val) {
    double t = window->t;
    double dt = (double)(float)(t - an->last);
    unsigned char v = *val;
    float p;
    if (dt > tenth_in_gui_text) {
        p = (float)v;
        an->pos = p;
    } else {
        p = an->pos;
        if (v) {
            if (threshold_1_in_gui_text_b > p) {
                p = (float)(dt / tenth_in_gui_text + (double)p);
                p = p < threshold_1_in_gui_text_b ? p : threshold_1_in_gui_text_b;
                an->pos = p;
            }
        } else if (p > 0.0f) {
            p = (float)(dt / divisor_neg_0_1_in_gui_text + (double)p);
            p = p > 0.0f ? p : 0.0f;
            an->pos = p;
        }
    }
    an->last = t;
    void *nvg = window->ig;
    float e = ease_curve(p);
    float k = mouse_over_rect_r(gui, r) ? gui_toggle_hover_grey_level : gui_toggle_idle_grey_level;
    V4 col = nvg_rgba_f(k, k, k, 1.0f);
    V4 track = nvg_lerp_rgba(col, *(V4 *)&gui->accent_color, e);
    nvg_FillColor(nvg, &track);
    draw_rect_rounded(window, r, corner_radius_10_in_gui_text);
    bool clicked = clicked_inside_r_frame(gui, *r);
    if (clicked) *val ^= 1;
    *r = inset_rect(*r, inset_6_in_gui_text_a);
    float h = r->h;
    float kk = gui_text_background_grey_level;
    V4 knob = nvg_rgba_f(kk, kk, kk, 1.0f);
    nvg_FillColor(nvg, &knob);
    nvg_BeginPath(nvg);
    float rad = (float)(int)h;
    float off = rad * factor_0_5_in_gui_text_a;
    float x = lerp_unclamped_float(off + r->x, r->x + r->w - off, e);
    nvg_Circle(nvg, x, off + r->y, rad);
    nvg_Fill(nvg);
    return clicked;
}

// ---- module header row: [+]/[-] expander box and name; returns the right edge of the clickable area ----
FN(R16c, make_rect, (float x, float y, float w, float h));
FN(float, nvg_text_bounds, (void *nvg, float x, float y, const char *text, const char *end, const float *bounds));   // nvgTextBounds
FN(void, nvg_StrokeColor, (void *nvg, V4 *c));
FN(void, nvg_MoveTo, (void *nvg, float x, float y));
FN(void, nvg_LineTo, (void *nvg, float x, float y));
FN(void, nvg_StrokeWidth, (void *nvg, float w));
FN(void, nvg_Stroke, (void *nvg));
extern "C" const float gui_text_gap, gui_text_idle_grey_level, gui_text_hover_grey_level, corner_radius_5_in_gui_text, inset_6_in_gui_text, gui_toggle_text_grey_level, one_in_gui_text,
    half_in_gui_text, stroke_width_2;
struct ModHdr { char padding_00[0x40]; unsigned char expanded; char padding_41[0x1b7]; void *settings; };
extern "C" float draw_module_header(GuiState *gui, WT2 *window, ModHdr *m, const R16c *r, float fsz, const V4 *col, const V4 *hcol,
                          const char *text) {
    void *nvg = window->ig;
    if (!m->settings) {
        nvg_FontSize(nvg, fsz);
        V4 c = *col;
        nvg_FillColor(nvg, &c);
        return draw_text(window, *r, 0, 0, (void *)text);
    }
    R16c box = make_rect(r->x, r->y, r->h, r->h);
    float gap = gui_text_gap;
    float tx = box.x + box.w + gap;
    float tw = nvg_text_bounds(nvg, tx, r->y, text, 0, (const float *)0);
    R16c lbl = make_rect(tx, r->y, tw, r->h);
    R16c hit = make_rect(r->x, r->y, box.w + lbl.w + gap, r->h);
    if (clicked_inside_r_frame(gui, hit)) m->expanded ^= 1;
    bool hov = mouse_over_rect_r(gui, &hit);
    nvg_FontSize(nvg, fsz);
    V4 c = *(hov ? hcol : col);
    nvg_FillColor(nvg, &c);
    draw_text(window, lbl, 0, 0, (void *)text);
    float k = hov ? gui_text_hover_grey_level : gui_text_idle_grey_level;
    V4 bc = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &bc);
    draw_rect_rounded(window, &box, corner_radius_5_in_gui_text);
    R16c in = inset_rect(box, inset_6_in_gui_text);
    float k2 = hov ? one_in_gui_text : gui_toggle_text_grey_level;
    V4 sc = nvg_rgba_f(k2, k2, k2, 1.0f);
    nvg_StrokeColor(nvg, &sc);
    nvg_BeginPath(nvg);
    float half = half_in_gui_text;
    nvg_MoveTo(nvg, in.x, in.h * half + in.y);
    nvg_LineTo(nvg, in.x + in.w, in.h * half + in.y);
    if (!m->expanded) {
        nvg_MoveTo(nvg, in.w * half + in.x, in.y);
        nvg_LineTo(nvg, half * in.w + in.x, in.y + in.h);
    }
    nvg_StrokeWidth(nvg, stroke_width_2);
    nvg_Stroke(nvg);
    return hit.x + hit.w;
}

// ---- key-bind button ("PRESS A KEY..." while capturing) and optional "HOLD" toggle ----
FN(void, set_draw_color, (void *window, unsigned col));
FN(bool, drag_owner_is, (GuiState *gui, void *object));
FN(void, begin_drag, (GuiState *gui, void *object, void *other_object));
FN(float, libc_ceilf, (float x));   // ceilf
FN(const char *, mouse_button_name, (int key));
FN(V4, nvg_trans_rgba_f, (V4 c, float k));
extern "C" float text_width_cache[];
extern "C" const char str_press_a_key[], str_hold_in_gui_text[];   // "PRESS A KEY...", "HOLD"
extern "C" const float rect_extent_12_in_gui_text, colour_alpha_0_15, colour_level_0_6_in_gui_text, corner_radius_5_in_gui_text_a, colour_level_1_in_gui_text_a, colour_level_0_866667_in_gui_text_a;
static inline __attribute__((always_inline)) V4 grey(float k) { return nvg_rgba_f(k, k, k, 1.0f); }
extern "C" void draw_module_description(GuiState *gui, WT2 *window, void *cbArg, void *target, int key, unsigned char *hold, const R16c *r) {
    void *nvg = window->ig;
    nvg_FontSize(nvg, rect_extent_12_in_gui_text);
    set_draw_color(window, 1);
    bool capturing = drag_owner_is(gui, target);
    float tw;
    if (capturing) {
        tw = libc_ceilf(nvg_text_bounds(nvg, 0.0f, 0.0f, str_press_a_key, 0, (const float *)0));
    } else {
        tw = text_width_cache[(unsigned)key];
        if (tw == 0.0f) {
            tw = libc_ceilf(nvg_text_bounds(nvg, 0.0f, 0.0f, mouse_button_name(key), 0, (const float *)0));
            text_width_cache[(unsigned)key] = tw;
        }
    }
    R16c btn = make_rect(r->x, r->y, tw + rect_extent_12_in_gui_text, r->h);
    V4 bg = key ? nvg_trans_rgba_f(*(V4 *)&gui->accent_color, colour_alpha_0_15) : nvg_trans_rgba_f(grey(colour_level_0_6_in_gui_text), colour_alpha_0_15);
    nvg_FillColor(nvg, &bg);
    draw_rect_rounded(window, &btn, corner_radius_5_in_gui_text_a);
    V4 tc;
    if (mouse_over_rect_r(gui, &btn)) tc = grey(colour_level_1_in_gui_text_a);
    else if (key) tc = *(V4 *)&gui->accent_color;
    else tc = grey(colour_level_0_6_in_gui_text);
    nvg_FillColor(nvg, &tc);
    draw_text(window, btn, 1, 1, (void *)(capturing ? str_press_a_key : mouse_button_name(key)));
    if (clicked_inside_r_frame(gui, btn)) begin_drag(gui, cbArg, target);
    if (!key) {
        if (hold) *hold = 0;
        return;
    }
    if (!hold) return;
    float hw = libc_ceilf(nvg_text_bounds(nvg, 0.0f, 0.0f, str_hold_in_gui_text, 0, (const float *)0));
    R16c hb = make_rect(btn.x + btn.w + corner_radius_5_in_gui_text_a, btn.y, hw + rect_extent_12_in_gui_text, btn.h);
    V4 hc;
    if (*hold) {
        hc = nvg_trans_rgba_f(grey(colour_level_0_866667_in_gui_text_a), colour_alpha_0_15);
    } else {
        if (!mouse_over_rect_r(gui, r)) return;
        hc = *hold == 1 ? nvg_trans_rgba_f(grey(colour_level_0_866667_in_gui_text_a), colour_alpha_0_15) : nvg_trans_rgba_f(grey(colour_level_0_6_in_gui_text), colour_alpha_0_15);
    }
    nvg_FillColor(nvg, &hc);
    draw_rect_rounded(window, &hb, corner_radius_5_in_gui_text_a);
    V4 ht = mouse_over_rect_r(gui, &hb) ? grey(colour_level_0_866667_in_gui_text_a) : grey(colour_level_0_6_in_gui_text);
    nvg_FillColor(nvg, &ht);
    draw_text(window, hb, 1, 1, (void *)str_hold_in_gui_text);
    if (clicked_inside_r_frame(gui, hb)) *hold ^= 1;
}
// labelled toggle row: label text, then the switch right of it
extern "C" const float row_height_20_in_gui_text, gui_label_text_grey_level, font_size_15_in_gui_text, rect_extent_neg_40_in_gui_text, rect_extent_40_in_gui_text;
FN(R16c, make_rect, (float x, float y, float w, float h));
FN(bool, draw_animated_description_local, (GuiState *gui, WT2 *window, R16c r, Anim *an, unsigned char *val)) __asm__("draw_animated_description");
extern "C" bool draw_toggle_row(GuiState *gui, WT2 *window, const char *label, Anim *an, unsigned char *val) {
    void *nvg = window->ig;
    R16c r = add_cell(gui, row_height_20_in_gui_text);
    float k = gui_label_text_grey_level;
    V4 c = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &c);
    nvg_FontSize(nvg, font_size_15_in_gui_text);
    draw_text(window, r, 0, 0, (void *)label);
    return draw_animated_description_local(gui, window, make_rect(r.x + r.w + rect_extent_neg_40_in_gui_text, r.y, rect_extent_40_in_gui_text, r.h), an, val);
}

// ---- float slider: label, track, animated fill + knob, drag/typing via draw_slider_track, value tooltip ----
struct SAnim { unsigned char active; char padding_01[3]; float pos; double last; };
FN(unsigned char, draw_slider_track, (GuiState *gui, R16c track, R16c hit, float lo, float hi, float step, float scale, SAnim *an,
                          float *val));
FN(float, limit_rotation_delta, (float lo, float hi, float x));   // clamp
FN(float, lerp_clamped_float, (float a, float b, float t));
FN(bool, dragged_item_is, (GuiState *gui, void *item));
FN(int, libc_snprintf, (char *buf, unsigned long long n, const char *fmt, ...));   // snprintf
extern "C" const float row_height_20_in_gui_text_a, colour_level_0_866667_in_gui_text_c, font_size_15_in_gui_text_b, offset_10_in_gui_text_a, offset_neg_2_in_gui_text, gui_description_idle_grey_level,
    gui_slider_hover_grey_level, two_in_gui_text, clamp_bound_1_in_gui_text_c, gui_slider_knob_radius;
extern "C" const double factor_0_5_in_gui_text, offset_neg_2, divisor_0_1_in_gui_text_a;
extern "C" const char fmt_float_2dp[];   // "%0.2f"
extern "C" char value_label_buffer[];
extern "C" unsigned char draw_slider_row(GuiState *gui, WT2 *window, const char *label, const char *fmt, SAnim *active, float lo, float hi,
                                  float step, float scale, float *val) {
    void *nvg = window->ig;
    R16c r = add_cell(gui, row_height_20_in_gui_text_a);
    V4 tc = nvg_rgba_f(colour_level_0_866667_in_gui_text_c, colour_level_0_866667_in_gui_text_c, colour_level_0_866667_in_gui_text_c, 1.0f);
    nvg_FillColor(nvg, &tc);
    nvg_FontSize(nvg, font_size_15_in_gui_text_b);
    draw_text(window, r, 0, 0, (void *)label);
    float y = r.y + offset_10_in_gui_text_a + offset_neg_2_in_gui_text;
    double half = (double)r.w * factor_0_5_in_gui_text;
    float x0 = (float)((double)r.x + half);
    float tw = (float)(half + offset_neg_2);
    R16c track = make_rect(x0, y, tw, 4.0f);
    R16c hr = r;
    float k = mouse_over_rect_r(gui, &hr) ? gui_slider_hover_grey_level : gui_description_idle_grey_level;
    V4 kc = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &kc);
    float rnd = two_in_gui_text;
    R16c tr = track;
    draw_rect_rounded(window, &tr, rnd);
    float t = limit_rotation_delta(0.0f, clamp_bound_1_in_gui_text_c, (*val - lo) / (hi - lo));
    float pos = lerp_clamped_float(active->pos, t, (float)((double)(float)(window->t - active->last) / divisor_0_1_in_gui_text_a));
    active->pos = pos;
    active->last = window->t;
    float fx = lerp_unclamped_float(x0, tw + x0, pos);
    R16c fill = make_rect(x0, y, fx - x0, 4.0f);
    V4 acc = *(V4 *)&gui->accent_color;
    nvg_FillColor(nvg, &acc);
    draw_rect_rounded(window, &fill, rnd);
    nvg_BeginPath(nvg);
    nvg_Circle(nvg, fx, rnd + y, gui_slider_knob_radius);
    nvg_Fill(nvg);
    unsigned char res = draw_slider_track(gui, track, r, lo, hi, step, scale, active, val);
    if (!active->active) {
        R16c h2 = r;
        if (!mouse_over_rect_r(gui, &h2) || gui->click_capture) return res;
    }
    const char *text;
    if (dragged_item_is(gui, val)) {
        text = gui->edit_text;
    } else {
        float v = *val;
        float shown = scale != 0.0f ? scale * v : v;
        text = value_label_buffer;
        libc_snprintf(value_label_buffer, 0x40, fmt ? fmt : fmt_float_2dp, (double)shown);
    }
    gui->toast_visible = 1;
    unsigned long long tn = libc_strlen(text);
    if (tn > 0x65) tn = 0x65;
    libc_memcpy(gui->toast_text, text, tn);
    gui->toast_text[tn] = 0;
    gui->toast_x = (float)(int)fx;
    gui->toast_y = (float)(int)y;
    return res;
}
