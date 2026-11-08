// GUI: controls - sliders, ranges, numeric fields, key pickers.
#include "slinky/engine.h"
#include "slinky/gui_state.h"

FN(void *, libc_memcpy, (void *dest, const void *src, unsigned long long n));
FN(unsigned long long, libc_strlen, (const char *str));
extern "C" void set_slider_text(char *o, void *, const char *src, int a, int b) {
    o[0x268] = 1;
    unsigned long long n = libc_strlen(src);
    if (n > 0x65) n = 0x65;
    libc_memcpy(o + 0x26a, src, n);
    o[0x26a + n] = 0;
    float *f = (float *)(o + 0x2d0);
    f[0] = (float)a;
    f[1] = (float)b;
}

// ---- slider: drag along the track or type a value (after a double click), rounded to step, clamped ----
struct R16g { float x, y, w, h; };
FN(unsigned char, clicked_inside_r_frame, (GuiState *gui, R16g r));
FN(void, start_dragging_item_b, (GuiState *gui, void *item, void *other_object));
FN(bool, dragged_item_is, (GuiState *gui, void *item));
FN(const char *, take_dragged_item, (GuiState *gui));
FN(void, numeric_text_filter, (char *buf, char character));
FN(float, libc_roundf, (float x));   // roundf
extern "C" const float tenth_b;
extern "C" const double factor_0_1_in_gui_controls;
extern "C" unsigned char draw_slider_track(GuiState *gui, const R16g *track, const R16g *hit, float lo, float hi, float step,
                                  float scale, unsigned char *active, float *val) {
    if (clicked_inside_r_frame(gui, *hit)) {
        *active = 1;
        if (gui->numeric_drag_enabled == 1) start_dragging_item_b(gui, (void *)numeric_text_filter, val);
    } else if (gui->click_capture == 0 && !dragged_item_is(gui, val)) {
        *active = 0;
    }
    unsigned char a = *active;
    if (a != 1) return a;
    float v;
    if (dragged_item_is(gui, val)) {
        const char *s = take_dragged_item(gui);
        if (!s) return a;
        int neg = *s == '-';
        const char *p = s + neg;
        int sign = -neg | 1;
        int ip = 0;
        char c = *p;
        while ((unsigned)(c - '0') <= 9) {
            ip = ip * 10 + (c - '0');
            c = *++p;
        }
        if ((unsigned char)(c | 2) == '.') {
            float f = 0.0f;
            float m = tenth_b;
            ++p;
            for (int d; (unsigned)(d = *p - '0') < 10; p++) {
                f += (float)d * m;
                m = (float)((double)m * factor_0_1_in_gui_controls);
            }
            v = ((float)ip + f) * (float)sign;
        } else {
            v = (float)(ip * sign);
        }
        if (scale != 0.0f) v /= scale;
    } else {
        float t = (gui->mouse_scene.x - track->x) / track->w;
        v = (hi - lo) * t + lo;
    }
    if (step != 0.0f) v = libc_roundf(v / step) * step;
    *val = v;
    if (lo >= v) *val = lo;
    else if (!(v < hi)) *val = hi;
    return a;
}

// ---- range slider: two knobs (each grabs on its side of the midpoint), they push each other ----
struct V4g { float x, y, z, w; };
struct GW14 { void *ig; double t; };
struct RAnim { unsigned char low_knob_active, high_knob_active; char padding_02[2]; float p0, p1; int padding_0c; double last; };
FN(R16g, add_cell, (GuiState *gui, float h));
FN(V4g, nvg_rgba_f, (float r, float g, float b, float a));
FN(void, nvg_FillColor, (void *nvg, V4g *c));
FN(void, nvg_FontSize, (void *nvg, float size));
FN(float, draw_text, (GW14 *window, R16g r, int align, int col, const char *text));
FN(R16g, make_rect, (float x, float y, float w, float h));
FN(bool, mouse_over_rect_r, (GuiState *gui, R16g r));
FN(void, draw_rect_rounded, (GW14 *window, R16g r, float round));
FN(float, limit_rotation_delta, (float lo, float hi, float x));
FN(float, lerp_clamped_float, (float a, float b, float t));
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_Circle, (void *nvg, float x, float y, float rad));
FN(void, nvg_Fill, (void *nvg));
FN(unsigned char, draw_slider_track_local, (GuiState *gui, R16g track, R16g hit, float lo, float hi, float step, float scale,
                           unsigned char *active, float *val)) __asm__("draw_slider_track");
FN(int, libc_snprintf, (char *buf, unsigned long long n, const char *fmt, ...));
extern "C" const float row_height_20_in_gui_controls, colour_level_0_866667_in_gui_controls, font_size_15_in_gui_controls, offset_10_in_gui_controls, offset_neg_2_in_gui_controls_b, gui_control_idle_grey_level,
    gui_control_hover_grey_level, two_in_gui_controls, clamp_bound_1_in_gui_controls, gui_range_slider_knob_radius, rect_extent_neg_6, gui_range_slider_padding;
extern "C" const double factor_0_5_in_gui_controls, offset_neg_2_in_gui_controls, divisor_0_1_in_gui_controls_a;
extern "C" const char fmt_float_2dp[], fmt_float_2dp_float_2dp[];   // "%0.2f", "%0.2f - %0.2f"
extern "C" char value_label_buffer[], range_label_buffer[];
static inline __attribute__((always_inline)) const char *val_text(GuiState *gui, float *item, const char *fmt, float scale) {
    if (dragged_item_is(gui, item)) return gui->edit_text;
    float x = *item;
    float s = scale != 0.0f ? scale * x : x;
    libc_snprintf(value_label_buffer, 0x40, fmt ? fmt : fmt_float_2dp, (double)s);
    return value_label_buffer;
}
extern "C" bool draw_range_slider_row(GuiState *gui, GW14 *window, const char *label, const char *fmt, const char *rfmt, RAnim *an, float lo,
                         float hi, float step, float scale, float *vlo, float *vhi) {
    void *nvg = window->ig;
    R16g r = add_cell(gui, row_height_20_in_gui_controls);
    V4g tc = nvg_rgba_f(colour_level_0_866667_in_gui_controls, colour_level_0_866667_in_gui_controls, colour_level_0_866667_in_gui_controls, 1.0f);
    nvg_FillColor(nvg, &tc);
    nvg_FontSize(nvg, font_size_15_in_gui_controls);
    draw_text(window, r, 0, 0, label);
    float y = r.y + offset_10_in_gui_controls + offset_neg_2_in_gui_controls_b;
    double half = (double)r.w * factor_0_5_in_gui_controls;
    float x0 = (float)((double)r.x + half);
    float tw = (float)(half + offset_neg_2_in_gui_controls);
    R16g track = make_rect(x0, y, tw, 4.0f);
    float k = mouse_over_rect_r(gui, r) ? gui_control_hover_grey_level : gui_control_idle_grey_level;
    V4g kc = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &kc);
    float rnd = two_in_gui_controls;
    draw_rect_rounded(window, track, rnd);
    float range = hi - lo;
    float t0 = limit_rotation_delta(0.0f, clamp_bound_1_in_gui_controls, (*vlo - lo) / range);
    float t1 = limit_rotation_delta(0.0f, clamp_bound_1_in_gui_controls, (*vhi - lo) / range);
    float dt = (float)((double)(float)(window->t - an->last) / divisor_0_1_in_gui_controls_a);
    an->p0 = lerp_clamped_float(an->p0, t0, dt);
    an->p1 = lerp_clamped_float(an->p1, t1, dt);
    an->last = window->t;
    float x1 = x0 + tw;
    float a = lerp_unclamped_float(x0, x1, an->p0);
    float b = lerp_unclamped_float(x0, x1, an->p1);
    R16g fill = make_rect(a, y, b - a, 4.0f);
    V4g acc = *(V4g *)&gui->accent_color;
    nvg_FillColor(nvg, &acc);
    draw_rect_rounded(window, fill, rnd);
    float ky = rnd + y, rad = gui_range_slider_knob_radius;
    nvg_BeginPath(nvg);
    nvg_Circle(nvg, a, ky, rad);
    nvg_Fill(nvg);
    nvg_BeginPath(nvg);
    nvg_Circle(nvg, b, ky, rad);
    nvg_Fill(nvg);
    float mid = (float)(int)((double)(a + b) * factor_0_5_in_gui_controls);
    float d = mid - x0;
    float pad = gui_range_slider_padding;
    R16g hl = make_rect(rect_extent_neg_6 + x0, r.y, d + pad, r.h);
    R16g hh = make_rect(mid, r.y, tw - d + pad, r.h);
    const char *text;
    int tx;
    bool changed;
    if (draw_slider_track_local(gui, track, hl, lo, hi, step, scale, &an->low_knob_active, vlo)) {
        if (*vlo > *vhi) *vhi = *vlo;
        text = val_text(gui, vlo, fmt, scale);
        tx = (int)a;
        changed = true;
    } else if (draw_slider_track_local(gui, track, hh, lo, hi, step, scale, &an->high_knob_active, vhi)) {
        if (*vlo > *vhi) *vlo = *vhi;
        text = val_text(gui, vhi, fmt, scale);
        tx = (int)b;
        changed = true;
    } else {
        if (!mouse_over_rect_r(gui, r)) return false;
        if (gui->click_capture == 1 && !an->low_knob_active && an->high_knob_active != 1) return false;
        float l = *vlo, h = *vhi;
        float ls = scale == 0.0f ? l : scale * l;
        float hs = scale == 0.0f ? h : scale * h;
        libc_snprintf(range_label_buffer, 0x40, rfmt ? rfmt : fmt_float_2dp_float_2dp, (double)ls, (double)hs);
        text = range_label_buffer;
        tx = (int)((double)(hl.x + hl.w + hh.x) * factor_0_5_in_gui_controls);
        changed = false;
    }
    gui->toast_visible = 1;
    unsigned long long tn = libc_strlen(text);
    if (tn > 0x65) tn = 0x65;
    libc_memcpy(gui->toast_text, text, tn);
    gui->toast_text[tn] = 0;
    gui->toast_x = (float)tx;
    gui->toast_y = (float)(int)y;
    return changed;
}

// ---- dropdown: expands in place (< 6 items) or opens the "Select item" search popup; optional dropdowns
// (index 0 = none) get a clear cross ----
struct DDrop { char padding_00[8]; double last; float h; unsigned char open; };
FN(void, nvg_StrokeColor, (void *nvg, V4g *c));
FN(void, nvg_StrokeWidth, (void *nvg, float w));
FN(void, draw_rect_stroked, (GW14 *window, R16g r, float round));
FN(V4g, nvg_trans_rgba_f, (V4g c, float k));
FN(void, nvg_MoveTo, (void *nvg, float x, float y));
FN(void, nvg_LineTo, (void *nvg, float x, float y));
FN(void, nvg_Stroke, (void *nvg));
FN(void, open_list_popup, (GuiState *gui, const char *title, void *sel, void *filter, void *pick, void *items, unsigned long long n));
FN(void, text_filter_alnum, (char *buf, char character));
FN(void, select_index, (int *out, int i, void *sel));
extern "C" const float twenty_in_gui_controls_a, colour_level_0_866667_in_gui_controls_a, font_size_15_in_gui_controls_a, colour_alpha_0_5_in_gui_controls, colour_level_0_25098_in_gui_controls_a, stroke_width_2_in_gui_controls_a,
    corner_radius_10_in_gui_controls_a, colour_level_0_933333_in_gui_controls, colour_level_0_345098_in_gui_controls, colour_level_1_in_gui_controls_a, colour_level_0_6_in_gui_controls, colour_level_0_733333_in_gui_controls;
extern "C" const double divisor_0_1_in_gui_controls_b, factor_0_5_in_gui_controls_a, factor_0_3_in_gui_controls;
extern "C" const char str_select_item[];   // "Select item"
static inline __attribute__((always_inline)) V4g grey14(float k) { return nvg_rgba_f(k, k, k, 1.0f); }
extern "C" unsigned char draw_dropdown_row(GuiState *gui, GW14 *window, const char *label, DDrop *dd, unsigned n, const char **items,
                                  int *sel, bool opt) {
    void *nvg = window->ig;
    float dt = (float)(window->t - dd->last);
    float target = dd->open ? (float)(28 * n) : twenty_in_gui_controls_a;
    float h = lerp_clamped_float(dd->h, target, (float)((double)dt / divisor_0_1_in_gui_controls_b));
    dd->h = h;
    dd->last = window->t;
    R16g r = add_cell(gui, h);
    R16g lr = r;
    lr.h = 20.0f;
    V4g tc = grey14(colour_level_0_866667_in_gui_controls_a);
    nvg_FillColor(nvg, &tc);
    nvg_FontSize(nvg, font_size_15_in_gui_controls_a);
    draw_text(window, lr, 0, 0, label);
    R16g box = make_rect((float)((double)r.w * factor_0_5_in_gui_controls_a + (double)r.x), r.y, r.w * colour_alpha_0_5_in_gui_controls, r.h);
    V4g oc = grey14(colour_level_0_25098_in_gui_controls_a);
    nvg_StrokeColor(nvg, &oc);
    nvg_StrokeWidth(nvg, stroke_width_2_in_gui_controls_a);
    draw_rect_stroked(window, box, corner_radius_10_in_gui_controls_a);
    if (dd->open) {
        float rh = box.h;
        V4g sc = nvg_trans_rgba_f(grey14(colour_level_0_25098_in_gui_controls_a), colour_alpha_0_5_in_gui_controls);
        nvg_StrokeColor(nvg, &sc);
        if (!n) return 0;
        rh /= (float)n;
        bool notopt = !opt;
        unsigned char done = 0;
        for (unsigned long long i = 0; i != n; i++) {
            R16g row = make_rect(box.x, (float)(unsigned)i * rh + box.y, box.w, rh);
            const char *name = items[i];
            bool hov = mouse_over_rect_r(gui, row);
            unsigned s = (unsigned)*sel;
            V4g c;
            if (i == s) c = *(V4g *)&gui->accent_color;
            else if (hov) c = grey14(i == 0 || notopt ? colour_level_1_in_gui_controls_a : colour_level_0_733333_in_gui_controls);
            else c = grey14(i != 0 || notopt ? colour_level_0_866667_in_gui_controls_a : colour_level_0_6_in_gui_controls);
            nvg_FillColor(nvg, &c);
            draw_text(window, row, 1, 0, name);
            if (!(done & 1)) {
                if (clicked_inside_r_frame(gui, row)) {
                    *sel = (int)i;
                    dd->open = 0;
                    done = 1;
                } else {
                    done = 0;
                }
            } else {
                done = 1;
            }
            if (i) {
                nvg_BeginPath(nvg);
                nvg_MoveTo(nvg, row.x, row.y);
                nvg_LineTo(nvg, row.x + row.w, row.y);
                nvg_Stroke(nvg);
            }
        }
        return done;
    }
    bool small = n < 6;
    bool hov = mouse_over_rect_r(gui, box);
    int s = *sel;
    if (opt && !small) {
        if (s) {
            float y = box.y, xh = box.h, x = box.w - xh + box.x;
            R16g xr = {x, y, xh, xh};
            bool xhov = mouse_over_rect_r(gui, xr);
            V4g xc = xhov ? nvg_rgba_f(colour_level_0_933333_in_gui_controls, colour_level_0_345098_in_gui_controls, colour_level_0_345098_in_gui_controls, 1.0f) : grey14(colour_level_0_25098_in_gui_controls_a);
            nvg_StrokeColor(nvg, &xc);
            float in = (float)((double)xh * factor_0_3_in_gui_controls);
            nvg_BeginPath(nvg);
            float x0 = x + in, y0 = y + in, x1 = xh + x - in, y1 = y + xh - in;
            nvg_MoveTo(nvg, x0, y0);
            nvg_LineTo(nvg, x1, y1);
            nvg_MoveTo(nvg, x1, y0);
            nvg_LineTo(nvg, x0, y1);
            nvg_Stroke(nvg);
            if (clicked_inside_r_frame(gui, xr)) {
                *sel = 0;
                s = 0;
            } else {
                s = *sel;
            }
            hov = hov & !xhov;
        } else {
            s = 0;
        }
    }
    if (!n) return 0;
    V4g c;
    if (opt && s == 0) c = grey14(hov ? colour_level_0_733333_in_gui_controls : colour_level_0_6_in_gui_controls);
    else c = grey14(hov ? colour_level_1_in_gui_controls_a : colour_level_0_866667_in_gui_controls_a);
    nvg_FillColor(nvg, &c);
    if ((unsigned)s >= n) s = 0;
    draw_text(window, box, 1, 0, items[(unsigned)s]);
    if (clicked_inside_r_frame(gui, box)) {
        if (n >= 6) open_list_popup(gui, str_select_item, sel, (void *)text_filter_alnum, (void *)select_index, items + 1, n - 1);
        else dd->open = 1;
    }
    return 0;
}
