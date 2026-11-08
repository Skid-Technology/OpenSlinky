// 0x16000
#include "slinky/engine.h"
#include "slinky/gui_state.h"
FN(void, set_popup_open, (void *x, bool b));
extern "C" void close_popup(void *x) { set_popup_open(x, true); }
extern "C" bool popup_is_open(GuiState *gui) { return gui->popup_kind_i != 0; }
extern "C" unsigned char popup_is_animating(GuiState *gui) { return (unsigned char)gui->popup_animating; }

struct Col3 { float r, g, b; };
struct Pick { void *ctx; char padding_08[0x18]; void (*add)(void *ctx, void *key, Col3 *c); };
extern "C" const float one_in_gui_popups, one_in_gui_popups_a;
extern "C" void pick_add_highlight(Pick *p, void *, void *key) {
    if (!key) return;
    void *ctx = p->ctx;
    auto add = p->add;

    alignas(16) Col3 c = {one_in_gui_popups, 0, 0};
    add(ctx, key, &c);
}
extern "C" void pick_add_shadow(Pick *p, void *key) {
    if (!key) return;
    void *ctx = p->ctx;
    auto add = p->add;

    alignas(16) Col3 c = {one_in_gui_popups_a, 0, 0};
    add(ctx, key, &c);
}
FN(unsigned long long, libc_strlen, (const char *str));
// numeric text filter: digits and , - .
extern "C" void numeric_text_filter(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    if (n == 100) return;
    unsigned char d = (unsigned char)(character - 0x30);

    unsigned char nd = d >= 10;

    unsigned char e = (unsigned char)(character - 0x2c);

    unsigned char np = e >= 3;

    if (np & nd) return;
    buf[n] = character;
}
struct R16 { long long a, b; };
FN(unsigned char, clicked_inside_r_frame, (void *gui, R16 r));
FN(void, start_dragging_item_b, (void *gui, void *item, void *other_object));
FN(bool, dragged_item_is, (GuiState *gui, void *item));
extern "C" __attribute__((noinline)) bool drag_item(GuiState *gui, void *, R16 *r, void *item, void *b) {
    if (clicked_inside_r_frame(gui, *r)) start_dragging_item_b(gui, item, b);
    return dragged_item_is(gui, b);
}

struct R16d { float x, y, w, h; };
struct V4b { float x, y, z, w; };
FN(float, draw_text, (void *window, R16d r, int a, int b, void *x));
FN(V4b, nvg_rgba_f, (float r, float g, float b, float a));
FN(void, draw_accent_line, (void *window, R16d r, V4b c, float a, float b));
extern "C" const float offset_1_in_gui_popups_b, gui_popup_title_grey_level, gui_popup_title_inset_a;
extern "C" __attribute__((noinline)) void draw_tracked_text(GuiState *gui, void *object, const R16d *rp) {
    R16d r = *rp;
    float f = draw_text(object, *rp, 1, 0, gui->edit_text) + offset_1_in_gui_popups_b;
    float k = gui_popup_title_grey_level;
    V4b c = nvg_rgba_f(k, k, k, 1.0f);
    R16d n = {f, r.y, r.w, r.h};
    draw_accent_line(object, n, c, gui_popup_title_inset_a, 1.0f);
}

extern "C" const float one_in_gui_popups_c, gui_popup_text_grey_level, gui_popup_title_inset_b;
extern "C" __attribute__((noinline)) void draw_clipped_text(GuiState *gui, void *object, const R16d *rp, int align) {
    float f = draw_text(object, *rp, align, 0, gui->edit_text);
    R16d r = *rp;
    f += one_in_gui_popups_c;
    float k = gui_popup_text_grey_level;
    V4b c = nvg_rgba_f(k, k, k, 1.0f);
    R16d n = {f, r.y, r.w, r.h};
    draw_accent_line(object, n, c, gui_popup_title_inset_b, 1.0f);
}
FN(void, set_popup_open, (void *x, bool b));
FN(double, time_now, ());
FN(void, start_dragging_item_b, (void *g, void *a, void *b));
// start a timed popup/drag payload
extern "C" void start_timed_popup_drag_payload(GuiState *gui, void *object, void *other_object, void *item, void *d) {
    set_popup_open(gui, true);
    double now = time_now();
    gui->popup_kind = 1;
    gui->popup_context = other_object;
    gui->popup_title = object;
    gui->popup_open_time = now;
    __builtin_memset(&gui->popup_opacity, 0, 0x10);
    gui->popup_callback = d;
    gui->popup_list_callback = item;
    __builtin_memset(&gui->popup_rgb.b, 0, 0x60);
    start_dragging_item_b(gui, item, &gui->popup_kind);
}

#include <stdarg.h>
FN(int, libc_vsnprintf, (char *buf, unsigned long long n, const char *fmt, va_list ap));
FN(void, end_drag, (void *gui));
extern "C" const char str_are_you_sure[];   // "Are you sure?"
// confirmation popup with a formatted message
extern "C" void confirmation_popup_formatted_message(GuiState *gui, void *object, void *other_object, const char *fmt, ...) {
    set_popup_open(gui, true);
    double now = time_now();
    gui->popup_kind = 3;
    gui->popup_context = object;
    gui->popup_title_text = str_are_you_sure;
    gui->popup_open_time = now;
    __builtin_memset(&gui->popup_opacity, 0, 0x10);
    gui->popup_callback = other_object;
    __builtin_memset(gui->popup_message, 0, 0x68);
    va_list ap;
    va_start(ap, fmt);
    libc_vsnprintf(gui->popup_message, 100, fmt, ap);
    va_end(ap);
}
extern "C" void open_list_popup(GuiState *gui, void *object, void *other_object, void *item, void *d0, void *d1, void *e) {
    set_popup_open(gui, true);
    double now = time_now();
    gui->popup_kind = 4;
    gui->popup_context = other_object;
    gui->popup_title = object;
    gui->popup_open_time = now;
    __builtin_memset(&gui->popup_opacity, 0, 0x10);
    gui->popup_callback = item;
    gui->popup_list_callback = d0;
    gui->popup_list_items = d1;
    gui->popup_list_extra = e;
    __builtin_memset(&gui->popup_list_state, 0, 0x50);
    start_dragging_item_b(gui, item, &gui->popup_kind);
    end_drag(gui);
}

// ---- popups: 1 = callback, 2 = colour picker, 3 = confirm, 4 = list picker ----
struct Col3b { float r, g, b; };
extern "C" unsigned long long gui_popups_recent_colour_ring_position;   // recent-colour ring position
extern "C" Col3b recent_colors_ring[8];
struct ListV { unsigned long long n, cap; void *data; };
static inline __attribute__((always_inline)) bool same(const Col3b &a, const Col3b &b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
// close the open popup (apply = run its callback with the result)
FN(void, libc_free, (void *ptr));
extern "C" __attribute__((noinline)) void set_popup_open(void *gv, bool apply) {
    GuiState *gui = (GuiState *)gv;
    gui->input_mode = 0;
    switch (gui->popup_kind_u) {
    case 0:
        return;
    case 1:
        if (apply) ((void (*)(void *, void *))gui->popup_callback)(gui->popup_context, 0);
        break;
    case 2: {
        if (apply) {
            Col3b c = *(Col3b *)&gui->popup_rgb;
            ((void (*)(void *, Col3b *))gui->popup_callback)(gui->popup_context, &c);
        }
        Col3b c = *(Col3b *)&gui->popup_rgb;
        unsigned long long p = gui_popups_recent_colour_ring_position;
        for (unsigned j = 0; j < 8; j++)
            if (same(c, recent_colors_ring[(p + 8 - j) & 7])) goto out;
        p = p ? p - 1 : 7;
        gui_popups_recent_colour_ring_position = p;
        recent_colors_ring[p] = c;
        break;
    }
    case 3:
        break;
    case 4: {
        if (apply) ((void (*)(void *, void *, void *))gui->popup_list_callback)(gui->popup_context, 0, 0);
        ListV *popup_list_view = ((ListV *)gui->popup_list_view);
        if (popup_list_view) {
            popup_list_view->n = 0;
            popup_list_view->cap = 0;
            libc_free(popup_list_view->data);
            popup_list_view->data = 0;
            libc_free(gui->popup_list_view);
            gui->popup_list_view = 0;
        }
        break;
    }
    }
out:
    gui->popup_kind = 0;
}
extern "C" const float half_in_gui_popups, offset_2_in_gui_popups, offset_4_in_gui_popups, divisor_6_in_gui_popups;   // 0.5, 2, 4, 6
// open the colour picker for colour c (keeps the RGB and its HSL)
extern "C" void open_colour_picker_colour_c(GuiState *gui, void *object, void *other_object, const Col3b *c, void *cb) {
    set_popup_open(gui, true);
    double now = time_now();
    Col3b in = *c;
    float r = c->r, gg = c->g, bb = c->b;
    float mx = __builtin_fmaxf(r, __builtin_fmaxf(gg, bb));
    float mn = __builtin_fminf(r, __builtin_fminf(gg, bb));
    float sum = mx + mn;
    float l = half_in_gui_popups * sum;
    float h = 0.0f, s = 0.0f;
    if (mx != mn) {
        float d = mx - mn;
        float denom = half_in_gui_popups < l ? offset_2_in_gui_popups - mx - mn : sum;
        if (mx == r) h = (gg - bb) / d + (gg < bb ? divisor_6_in_gui_popups : 0.0f);
        else if (mx == gg) h = (bb - r) / d + offset_2_in_gui_popups;
        else if (mx == bb) h = (r - gg) / d + offset_4_in_gui_popups;
        else h = l;
        s = d / denom;
        h = h / divisor_6_in_gui_popups;
    }
    gui->popup_kind = 2;
    gui->popup_context = other_object;
    gui->popup_title = object;
    gui->popup_open_time = now;
    __builtin_memset(&gui->popup_opacity, 0, 0x10);
    gui->popup_callback = cb;
    *(Col3b *)&gui->popup_rgb = in;
    gui->popup_hsl.h = h;
    gui->popup_hsl.s = s;
    gui->popup_hsl.l = l;
    __builtin_memset(&gui->popup_list_state, 0, 0x50);
}

// ---- draw the open popup: title bar with close cross, then by kind (popup_kind): 1 text prompt, 2 colour
// picker (hex field, HSL sliders, recent colours), 3 confirm, 4 searchable list. Closes it when done. ----
struct GW16 { void *ig; double t; };
struct Grad { float v[19]; };
struct Hit { unsigned long long idx; const char *name; };
FN(void, pop_layout_row, (GuiState *gui, int n));
FN(R16d, add_cell, (GuiState *gui, float h));
FN(void, nvg_FillColor, (void *nvg, V4b *c));
FN(void, nvg_StrokeColor, (void *nvg, V4b *c));
FN(void, nvg_StrokeWidth, (void *nvg, float w));
FN(void, nvg_FontSize, (void *nvg, float size));
FN(void, draw_rect_rounded, (GW16 *window, R16d r, float round));
FN(void, draw_rect_stroked, (GW16 *window, R16d r, float round));
FN(void, begin_content, (GW16 *window, GuiState *gui, R16d r, float pad, float f));
FN(bool, mouse_over_rect_r, (GuiState *gui, R16d r));
FN(unsigned char, point_in_rect_local, (void *gui, R16d r)) __asm__("clicked_inside_r_frame");
FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_MoveTo, (void *nvg, float x, float y));
FN(void, nvg_LineTo, (void *nvg, float x, float y));
FN(void, nvg_Stroke, (void *nvg));
FN(void, nvg_Fill, (void *nvg));
FN(R16d, inset_rect, (R16d r, float d));
FN(void *, take_dragged_item, (GuiState *gui));
FN(char *, dragged_payload, (GuiState *gui));
FN(R16d, make_rect, (float x, float y, float w, float h));
FN(V4b, nvg_rgb_f, (float r, float g, float b));
FN(V4b, nvg_hsl, (float h, float s, float l));
FN(V4b, nvg_lerp_rgba, (V4b a, V4b b, float t));
FN(Grad, nvg_linear_gradient, (void *nvg, float x0, float y0, float x1, float y1, V4b c0, V4b c1));
FN(void, nvg_FillPaint, (void *nvg, Grad g));
FN(void, nvg_RoundedRectVarying, (void *nvg, float x, float y, float w, float h, float r0, float r1, float r2, float r3));
FN(void, nvg_Rect, (void *nvg, float x, float y, float w, float h));
FN(void, nvg_RoundedRect, (void *nvg, float x, float y, float w, float h, float r));
FN(float, libc_ceilf, (float x));
FN(float, libc_floorf, (float x));
FN(float, draw_section_label, (GuiState *gui, GW16 *window, const char *label));
FN(bool, is_packet_cancelled, (GuiState *gui, GW16 *window, R16d r, unsigned idx, V4b *col));
FN(void, hex_colour_text_filter, (char *buf, char character));
FN(float, end_layout_row, (GW16 *window, GuiState *gui));
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(unsigned char, vector_grow, (void *vec, unsigned long long elem, unsigned long long n, void *cap));
FN(int, libc_snprintf, (char *buf, unsigned long long n, const char *fmt, ...));
extern "C" const float colour_level_1_in_gui_popups_d, colour_level_0_0784314_in_gui_popups, twenty_in_gui_popups, corner_radius_8_in_gui_popups_c, font_size_16_in_gui_popups, colour_level_0_866667_in_gui_popups_c, colour_level_0_6_in_gui_popups_a,
    colour_level_0_933333_in_gui_popups, colour_level_0_345098_in_gui_popups, stroke_width_2_in_gui_popups_a, row_height_30, font_size_15_in_gui_popups, colour_level_0_25098_in_gui_popups_a, inset_4_in_gui_popups_a, row_height_25,
    factor_0_5_in_gui_popups_a, colour_level_0_278431_in_gui_popups, corner_radius_5_in_gui_popups, row_height_24, colour_byte_max, gui_popup_button_hover_grey_level, divisor_6_in_gui_popups_a, offset_neg_56,
    factor_0_125_b, gui_popup_hover_grey_level;
extern "C" const double divisor_0_3_in_gui_popups, factor_0_2_in_gui_popups, threshold_0_25, divisor_255;
extern "C" const char str_hue[], str_saturation[], str_lightness[], str_recently_used[], str_str[], fmt_hex2_hex2_hex2[],
    str_continue[], str_d_more[];
extern "C" char color_hex_string_buffer[];
static inline __attribute__((always_inline)) unsigned char lower16(unsigned char c) {
    return (unsigned char)(c - 'A') < 26 ? (unsigned char)(c | 0x20) : c;
}
static inline __attribute__((always_inline)) unsigned char hexval(signed char c) {
    return (unsigned char)(c + (c >= 'a' ? 0xa9 : 0xd0));
}
// popup_rgb -> popup_hsl
static inline __attribute__((always_inline)) void rgb_to_hsl(GuiState *gui) {
    float r = gui->popup_rgb.r, gg = gui->popup_rgb.g, bb = gui->popup_rgb.b;
    float mx = __builtin_fmaxf(__builtin_fmaxf(bb, gg), r);
    float mn = __builtin_fminf(__builtin_fminf(bb, gg), r);
    float sum = mx + mn;
    float l = factor_0_5_in_gui_popups_a * sum;
    float h = 0.0f, s = 0.0f;
    if (mx != mn) {
        float d = mx - mn;
        float denom = factor_0_5_in_gui_popups_a < l ? stroke_width_2_in_gui_popups_a - mx - mn : sum;
        if (mx == r) h = (gg - bb) / d + (gg < bb ? divisor_6_in_gui_popups_a : 0.0f);
        else if (mx == gg) h = (bb - r) / d + stroke_width_2_in_gui_popups_a;
        else if (mx == bb) h = (r - gg) / d + inset_4_in_gui_popups_a;
        else h = l;
        s = d / denom;
        h = h / divisor_6_in_gui_popups_a;
    }
    gui->popup_hsl.h = h;
    gui->popup_hsl.s = s;
    gui->popup_hsl.l = l;
}
// one HSL slider: gradient from c(t=0) to c(t=1) with the knob at the current value
static inline __attribute__((always_inline)) bool hsl_bar(GuiState *gui, GW16 *window, unsigned idx, V4b c0, V4b c1, float t) {
    V4b mid = nvg_lerp_rgba(c0, c1, t);
    void *nvg = window->ig;
    R16d r = add_cell(gui, font_size_15_in_gui_popups);
    nvg_FillPaint(nvg, nvg_linear_gradient(nvg, r.x, r.y, r.w + r.x, r.h + r.y, c0, c1));
    draw_rect_rounded(window, r, corner_radius_5_in_gui_popups);
    return is_packet_cancelled(gui, window, r, idx, &mid);
}
extern "C" void close_popup_animated(GuiState *gui, GW16 *window) {
    gui->popup_animating = 1;
    float op = (float)((double)gui->popup_opacity + (window->t - gui->popup_open_time) / divisor_0_3_in_gui_popups);
    gui->popup_opacity = colour_level_1_in_gui_popups_d < op ? colour_level_1_in_gui_popups_d : op;
    pop_layout_row(gui, 1);
    R16d box = add_cell(gui, gui->popup_height);
    void *nvg = window->ig;
    V4b bg = nvg_rgba_f(colour_level_0_0784314_in_gui_popups, colour_level_0_0784314_in_gui_popups, colour_level_0_0784314_in_gui_popups, 1.0f);
    nvg_FillColor(nvg, &bg);
    draw_rect_rounded(window, box, twenty_in_gui_popups);
    begin_content(window, gui, box, corner_radius_8_in_gui_popups_c, 12.0f);
    pop_layout_row(gui, 1);
    void *tig = window->ig;
    R16d t = add_cell(gui, twenty_in_gui_popups);
    nvg_FontSize(tig, font_size_16_in_gui_popups);
    V4b tc = nvg_rgba_f(colour_level_0_866667_in_gui_popups_c, colour_level_0_866667_in_gui_popups_c, colour_level_0_866667_in_gui_popups_c, 1.0f);
    nvg_FillColor(tig, &tc);
    draw_text(window, t, 0, 0, gui->popup_title);
    // close cross in the title bar's right square
    float cx = t.w - t.h + t.x;
    R16d xr = {cx, t.y, t.h, t.h};
    V4b xc = mouse_over_rect_r(gui, xr) ? nvg_rgba_f(colour_level_0_933333_in_gui_popups, colour_level_0_345098_in_gui_popups, colour_level_0_345098_in_gui_popups, 1.0f)
                             : nvg_rgba_f(colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, 1.0f);
    nvg_StrokeColor(tig, &xc);
    nvg_StrokeWidth(tig, stroke_width_2_in_gui_popups_a);
    float in = (float)((double)t.h * factor_0_2_in_gui_popups);
    nvg_BeginPath(tig);
    float x0 = cx + in, y0 = t.y + in;
    nvg_MoveTo(tig, x0, y0);
    float x1 = t.h + cx - in, y1 = t.y + t.h - in;
    nvg_LineTo(tig, x1, y1);
    nvg_MoveTo(tig, x1, y0);
    nvg_LineTo(tig, x0, y1);
    nvg_Stroke(tig);
    bool closed = point_in_rect_local(gui, R16d{cx, t.y, t.h, t.h});
    int res = 1;   // 0 keep open, 1 close (cancel), 2 close (done)
    void *key = &gui->popup_kind;
    switch (gui->popup_kind_i) {
    case 1: {   // text prompt -> cb(ctx, text)
        void *fig = window->ig;
        R16d f = add_cell(gui, row_height_30);
        nvg_FontSize(fig, font_size_15_in_gui_popups);
        V4b oc = nvg_rgba_f(colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, 1.0f);
        nvg_StrokeColor(fig, &oc);
        nvg_StrokeWidth(fig, stroke_width_2_in_gui_popups_a);
        draw_rect_stroked(window, f, corner_radius_8_in_gui_popups_c);
        res = 1;
        if (!dragged_item_is(gui, key)) break;
        f = inset_rect(f, inset_4_in_gui_popups_a);
        draw_tracked_text(gui, window, &f);
        void *s = take_dragged_item(gui);
        if (!s) {
            res = 0;
            break;
        }
        ((void (*)(void *, void *))gui->popup_callback)(gui->popup_context, s);
        res = 2;
        break;
    }
    case 2: {   // colour picker
        void *pig = window->ig;
        R16d f = add_cell(gui, row_height_24);
        bool hov = mouse_over_rect_r(gui, f);
        float fh = f.h;
        V4b sw = nvg_rgb_f(gui->popup_rgb.r, gui->popup_rgb.g, gui->popup_rgb.b);
        nvg_FillColor(pig, &sw);
        draw_rect_rounded(window, R16d{f.x, f.y, fh, fh}, corner_radius_5_in_gui_popups);
        V4b oc = nvg_rgba_f(colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, 1.0f);
        nvg_StrokeColor(pig, &oc);
        nvg_StrokeWidth(pig, stroke_width_2_in_gui_popups_a);
        nvg_Stroke(pig);
        nvg_FontSize(pig, twenty_in_gui_popups);
        V4b hc = nvg_rgba_f(colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, 1.0f);
        nvg_FillColor(pig, &hc);
        float hx = f.x + fh + corner_radius_8_in_gui_popups_c;
        float hw = f.w - (f.x - hx);
        float ex = draw_text(window, R16d{hx, f.y, hw, f.h}, 0, 0, (void *)str_str /* "#" */);
        hx -= ex;
        hw -= hx;
        R16d hr = {ex, f.y, hw, f.h};
        R16 ed;
        __builtin_memcpy(&ed, &f, sizeof ed);
        if (drag_item(gui, window, &ed, (void *)hex_colour_text_filter, &gui->popup_callback)) {
            V4b wc = nvg_rgba_f(colour_level_1_in_gui_popups_d, colour_level_1_in_gui_popups_d, colour_level_1_in_gui_popups_d, 1.0f);
            nvg_FillColor(pig, &wc);
            draw_clipped_text(gui, window, &hr, 0);
            const char *s = (const char *)take_dragged_item(gui);
            if (s) {
                if (libc_strlen(s) > 5) {
                    gui->popup_rgb.r =
                        (float)((double)(hexval(s[0]) << 4 | hexval(s[1])) / divisor_255);
                    gui->popup_rgb.g =
                        (float)((double)(hexval(s[2]) << 4 | hexval(s[3])) / divisor_255);
                    gui->popup_rgb.b =
                        (float)((double)(hexval(s[4]) << 4 | hexval(s[5])) / divisor_255);
                }
                rgb_to_hsl(gui);
            }
        } else {
            float k = colour_byte_max;
            libc_snprintf(color_hex_string_buffer, 7, fmt_hex2_hex2_hex2 /* "%02x%02x%02x" */,
                     (unsigned char)(long long)(gui->popup_rgb.r * k),
                     (unsigned char)(long long)(gui->popup_rgb.g * k),
                     (unsigned char)(long long)(k * gui->popup_rgb.b));
            float c = hov ? gui_popup_button_hover_grey_level : colour_level_0_6_in_gui_popups_a;
            V4b tc2 = nvg_rgba_f(c, c, c, 1.0f);
            nvg_FillColor(pig, &tc2);
            draw_text(window, hr, 0, 0, color_hex_string_buffer);
        }
        // hue: six gradient segments, rounded at the ends
        draw_section_label(gui, window, str_hue /* "Hue" */);
        void *hig = window->ig;
        R16d hb = add_cell(gui, font_size_15_in_gui_popups);
        float seg = hb.w / divisor_6_in_gui_popups_a;
        float sw2 = libc_ceilf(seg);
        for (unsigned long long k = 0; k != 6; k++) {
            float sx = libc_floorf((float)k * seg + hb.x);
            V4b c0 = nvg_hsl((float)k / divisor_6_in_gui_popups_a, gui->popup_hsl.s, gui->popup_hsl.l);
            V4b c1 = nvg_hsl((float)(k + 1) / divisor_6_in_gui_popups_a, gui->popup_hsl.s, gui->popup_hsl.l);
            Grad gr = nvg_linear_gradient(hig, sx, hb.y, seg + sx, hb.y, c0, c1);
            nvg_BeginPath(hig);
            if (k == 5) nvg_RoundedRectVarying(hig, sx, hb.y, sw2, hb.h, 0.0f, 5.0f, 5.0f, 0.0f);
            else if (k == 0) nvg_RoundedRectVarying(hig, sx, hb.y, sw2, hb.h, 5.0f, 0.0f, 0.0f, 5.0f);
            else nvg_Rect(hig, sx, hb.y, sw2, hb.h);
            nvg_FillPaint(hig, gr);
            nvg_Fill(hig);
        }
        V4b cur = nvg_hsl(gui->popup_hsl.h, gui->popup_hsl.s, gui->popup_hsl.l);
        bool ch = is_packet_cancelled(gui, window, hb, 0, &cur);
        draw_section_label(gui, window, str_saturation /* "Saturation" */);
        bool cs = hsl_bar(gui, window, 1, nvg_hsl(gui->popup_hsl.h, 0.0f, gui->popup_hsl.l),
                          nvg_hsl(gui->popup_hsl.h, colour_level_1_in_gui_popups_d, gui->popup_hsl.l), gui->popup_hsl.s);
        draw_section_label(gui, window, str_lightness /* "Lightness" */);
        bool cl = hsl_bar(gui, window, 2, nvg_hsl(gui->popup_hsl.h, gui->popup_hsl.s, 0.0f),
                          nvg_hsl(gui->popup_hsl.h, gui->popup_hsl.s, colour_level_1_in_gui_popups_d), gui->popup_hsl.l);
        if (ch || cs || cl) {
            V4b c = nvg_hsl(gui->popup_hsl.h, gui->popup_hsl.s, gui->popup_hsl.l);
            __builtin_memcpy(gui->popup_message, &c, 8);
            gui->popup_rgb.b = c.z;
        }
        // recently used colours (ring buffer, newest first)
        draw_section_label(gui, window, str_recently_used /* "Recently used" */);
        void *rig = window->ig;
        R16d rr = add_cell(gui, row_height_25);
        float x = rr.x;
        float sz = (rr.w + offset_neg_56) * factor_0_125_b;
        nvg_StrokeWidth(rig, stroke_width_2_in_gui_popups_a);
        float step = corner_radius_8_in_gui_popups_c + sz;
        unsigned long long pick = 8;
        for (long long i = -8; i; i++) {
            unsigned long long p = gui_popups_recent_colour_ring_position + i + 8;
            unsigned long long idx = p < 8 ? p : gui_popups_recent_colour_ring_position + i;
            R16d cell = make_rect(x, rr.y, sz, rr.h);
            float k = mouse_over_rect_r(gui, cell) ? gui_popup_hover_grey_level : colour_level_0_25098_in_gui_popups_a;
            V4b oc2 = nvg_rgba_f(k, k, k, 1.0f);
            nvg_StrokeColor(rig, &oc2);
            Col3b *e = &recent_colors_ring[idx];
            if (point_in_rect_local(gui, cell)) pick = idx;
            nvg_BeginPath(rig);
            nvg_RoundedRect(rig, cell.x, cell.y, cell.w, cell.h, 5.0f);
            V4b fc = nvg_rgb_f(e->r, e->g, e->b);
            nvg_FillColor(rig, &fc);
            nvg_Fill(rig);
            nvg_Stroke(rig);
            x += step;
        }
        if (pick != 8) {
            *(Col3b *)&gui->popup_rgb = recent_colors_ring[pick];
            rgb_to_hsl(gui);
        }
        res = 0;
        break;
    }
    case 3: {   // confirm -> cb(ctx)
        void *cig = window->ig;
        R16d f = add_cell(gui, row_height_30);
        nvg_FontSize(cig, font_size_15_in_gui_popups);
        V4b mc = nvg_rgba_f(colour_level_0_866667_in_gui_popups_c, colour_level_0_866667_in_gui_popups_c, colour_level_0_866667_in_gui_popups_c, 1.0f);
        nvg_FillColor(cig, &mc);
        draw_text(window, f, 0, 0, gui->popup_message);
        R16d b = add_cell(gui, row_height_30);
        V4b bc = mouse_over_rect_r(gui, b) ? nvg_rgba_f(colour_level_0_933333_in_gui_popups, colour_level_0_345098_in_gui_popups, colour_level_0_345098_in_gui_popups, 1.0f)
                                : nvg_rgba_f(colour_level_0_866667_in_gui_popups_c, colour_level_0_278431_in_gui_popups, colour_level_0_278431_in_gui_popups, 1.0f);
        nvg_FillColor(cig, &bc);
        draw_rect_rounded(window, b, corner_radius_5_in_gui_popups);
        V4b wc = nvg_rgba_f(colour_level_1_in_gui_popups_d, colour_level_1_in_gui_popups_d, colour_level_1_in_gui_popups_d, 1.0f);
        nvg_FillColor(cig, &wc);
        draw_text(window, b, 1, 0, (void *)str_continue /* "Continue" */);
        res = 0;
        if (!point_in_rect_local(gui, b)) break;
        ((void (*)(void *))gui->popup_callback)(gui->popup_context);
        res = 2;
        break;
    }
    case 4: {   // searchable list -> cb(ctx, index, name)
        R16d f = add_cell(gui, row_height_30);
        void *lig = window->ig;
        nvg_FontSize(lig, font_size_15_in_gui_popups);
        V4b oc = nvg_rgba_f(colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, colour_level_0_25098_in_gui_popups_a, 1.0f);
        nvg_StrokeColor(lig, &oc);
        nvg_StrokeWidth(lig, stroke_width_2_in_gui_popups_a);
        draw_rect_stroked(window, f, corner_radius_8_in_gui_popups_c);
        if (!dragged_item_is(gui, key)) break;
        f = inset_rect(f, inset_4_in_gui_popups_a);
        draw_tracked_text(gui, window, &f);
        const char *q = dragged_payload(gui);
        double now = window->t;
        if (!(threshold_0_25 > now - gui->popup_list_state)) {   // rebuild the matches at most every 0.25 s
            ListV *popup_list_view = ((ListV *)gui->popup_list_view);
            if (!popup_list_view) {
                popup_list_view = (ListV *)libc_calloc(1, 0x18);
                gui->popup_list_view = popup_list_view;
                if (!popup_list_view) goto shown;
            }
            popup_list_view->n = 0;
            popup_list_view->cap = 0;
            libc_free(popup_list_view->data);
            popup_list_view->data = 0;
            gui->popup_list_count = 0;
            for (unsigned long long k = 0; k < gui->popup_list_extra_u; k++) {
                const char *it = (((const char **)gui->popup_list_items))[k];
                unsigned long long il = libc_strlen(it), ql = libc_strlen(q);
                if (il < ql) continue;
                if (ql) {   // case-insensitive substring
                    for (unsigned long long st = 0;; st++) {
                        unsigned long long p = 0;
                        while (lower16(it[st + p]) == lower16(q[p]))
                            if (++p == ql) goto match;
                        if (st + 1 > il - ql) goto next;
                    }
                }
            match : {
                ListV *m = ((ListV *)gui->popup_list_view);
                if (m->n >= 0x1e) {
                    (gui->popup_list_count)++;
                    continue;
                }
                if (!vector_grow(&m->data, 0x10, m->n, &m->cap)) continue;
                Hit *h = &((Hit *)m->data)[m->n];
                h->idx = k;
                h->name = it;
                m->n++;
            }
            next:;
            }
            gui->popup_list_state = now;
        }
    shown : {
        if (!gui->popup_list_view) {
            res = 0;
            break;
        }
        unsigned long long cnt = (((ListV *)gui->popup_list_view))->n;
        if (!cnt) {
            res = 0;
            break;
        }
        void *tig2 = window->ig;
        nvg_FontSize(tig2, font_size_16_in_gui_popups);
        const char *chosen = 0;
        unsigned long long cidx = 0;
        R16d row = {};
        for (unsigned long long i = 0; i != cnt; i++) {   // two columns
            float cx2, cw;
            if (i & 1) {
                cw = row.w * factor_0_5_in_gui_popups_a;
                cx2 = row.x + cw;
            } else {
                row = add_cell(gui, row_height_25);
                cw = row.w * factor_0_5_in_gui_popups_a;
                cx2 = row.x;
            }
            R16d cell = {cx2, row.y, cw, row.h};
            float k = mouse_over_rect_r(gui, cell) ? colour_level_1_in_gui_popups_d : colour_level_0_866667_in_gui_popups_c;
            V4b ic = nvg_rgba_f(k, k, k, 1.0f);
            nvg_FillColor(tig2, &ic);
            Hit *h = &((Hit *)(((ListV *)gui->popup_list_view))->data)[i];
            unsigned long long hi = h->idx;
            const char *hn = h->name;
            draw_text(window, cell, 0, 0, (void *)hn);
            if (point_in_rect_local(gui, cell)) {
                chosen = hn;
                cidx = hi;
            }
        }
        unsigned long long more = gui->popup_list_count;
        if (more) {
            char buf[0xe];
            libc_snprintf(buf, 0xe, str_d_more /* "+%d more..." */, more < 0x270f ? more : 0x270f);
            R16d mr = add_cell(gui, row_height_25);
            V4b mc = nvg_rgba_f(colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, colour_level_0_6_in_gui_popups_a, 1.0f);
            nvg_FillColor(tig2, &mc);
            nvg_FontSize(tig2, font_size_15_in_gui_popups);
            draw_text(window, mr, 0, 0, buf);
        }
        res = 0;
        if (!chosen) break;
        ((void (*)(void *, unsigned long long, const char *))gui->popup_list_callback)(gui->popup_context, cidx, chosen);
        res = 2;
        break;
    }
    }
    }
    gui->popup_height = end_layout_row(window, gui);
    gui->popup_animating = 0;
    gui->popup_open_time = window->t;
    if (res) set_popup_open(gui, res == 1);
    else if (closed) set_popup_open(gui, true);
}

// ---- selectable label: rounded box (highlighted when `hot`), text centred; returns whether it was clicked ----
extern "C" const float colour_level_0_25098_in_gui_popups, corner_radius_8_in_gui_popups, gui_popup_disabled_grey_level, gui_popup_enabled_grey_level, font_size_15_in_gui_popups_a;
extern "C" bool draw_icon_button(GuiState *gui, GW16 *window, const char *text, bool selectable, const R16d *rect) {
    void *nvg = window->ig;
    bool clicked = selectable ? point_in_rect_local(gui, *rect) : false;
    V4b bg = nvg_rgba_f(colour_level_0_25098_in_gui_popups, colour_level_0_25098_in_gui_popups, colour_level_0_25098_in_gui_popups, 1.0f);
    nvg_FillColor(nvg, &bg);
    draw_rect_rounded(window, *rect, corner_radius_8_in_gui_popups);
    float k = selectable ? gui_popup_enabled_grey_level : gui_popup_disabled_grey_level;
    V4b fg = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &fg);
    nvg_FontSize(nvg, font_size_15_in_gui_popups_a);
    draw_text(window, *rect, 1, 0, (void *)text);
    return clicked;
}
