// GUI: small helpers - text buffers and formatting.
#include "slinky/engine.h"
#include "slinky/gui_state.h"
extern "C" void select_index(int *out, int i, void *sel) { if (sel) *out = i + 1; }
struct C12 { long long a; int b; };
extern "C" void copy_pair12(C12 *o, const C12 *i) { o->b = i->b; o->a = i->a; }

FN(unsigned long long, libc_strlen, (const char *str));
extern "C" void text_filter_alnum(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    if (n > 99) return;
    unsigned char u = (unsigned char)((character & 0xdf) - 0x41);
    asm("" : "+r"(u));
    if (u >= 0x1a) {
        unsigned char d = (unsigned char)(character - 0x30);
    asm("" : "+r"(d));
        unsigned char notdigit = d >= 10;
    asm("" : "+r"(notdigit));
        unsigned char notspace = character != 0x20;
    asm("" : "+r"(notspace));
        if (notdigit & notspace) return;
    }
    buf[n] = character;
}

// ---- colour setting row: label, swatch (colour fading to 25 % alpha), click opens the colour picker ----
struct R15 { float x, y, w, h; };
struct V15 { float x, y, z, w; };
struct Grad15 { float v[19]; };
struct Col15 { float r, g, b; };
struct GW15 { void *ig; double t; };
FN(R15, add_cell, (GuiState *gui, float h));
FN(V15, nvg_rgba_f, (float r, float g, float b, float a));
FN(V15, nvg_rgb_f, (float r, float g, float b));
FN(void, nvg_FillColor, (void *nvg, V15 *c));
FN(void, nvg_StrokeColor, (void *nvg, V15 *c));
FN(void, nvg_StrokeWidth, (void *nvg, float w));
FN(void, nvg_FontSize, (void *nvg, float size));
FN(float, draw_text, (GW15 *window, R15 r, int align, int col, const char *text));
FN(R15, make_rect, (float x, float y, float w, float h));
FN(void, draw_rect_stroked, (GW15 *window, R15 r, float round));
FN(void, draw_rect_rounded, (GW15 *window, R15 r, float round));
FN(Grad15, nvg_linear_gradient, (void *nvg, float x0, float y0, float x1, float y1, V15 c0, V15 c1));
FN(void, nvg_FillPaint, (void *nvg, Grad15 g));
FN(unsigned char, clicked_inside_r_frame, (GuiState *gui, R15 r));
FN(void, open_colour_picker_colour_c, (GuiState *gui, const char *label, Col15 *col, const Col15 *c, void *cb));
extern "C" const float row_height_20_in_gui_helpers, colour_level_0_866667_in_gui_helpers, font_size_15_in_gui_helpers, rect_extent_neg_40_in_gui_helpers, rect_extent_40_in_gui_helpers, colour_level_0_25098_in_gui_helpers,
    stroke_width_2_in_gui_helpers, ten_in_gui_helpers, factor_0_5_in_gui_helpers_b;
extern "C" const double factor_0_5_in_gui_helpers_a;
extern "C" unsigned char draw_color_swatch_row(GuiState *gui, GW15 *window, const char *label, Col15 *col) {
    void *nvg = window->ig;
    R15 r = add_cell(gui, row_height_20_in_gui_helpers);
    R15 lr = r;
    lr.h = 20.0f;
    V15 tc = nvg_rgba_f(colour_level_0_866667_in_gui_helpers, colour_level_0_866667_in_gui_helpers, colour_level_0_866667_in_gui_helpers, 1.0f);
    nvg_FillColor(nvg, &tc);
    nvg_FontSize(nvg, font_size_15_in_gui_helpers);
    draw_text(window, lr, 0, 0, label);
    R15 sw = make_rect(r.x + r.w + rect_extent_neg_40_in_gui_helpers, r.y, rect_extent_40_in_gui_helpers, 20.0f);
    V15 oc = nvg_rgba_f(colour_level_0_25098_in_gui_helpers, colour_level_0_25098_in_gui_helpers, colour_level_0_25098_in_gui_helpers, 1.0f);
    nvg_StrokeColor(nvg, &oc);
    nvg_StrokeWidth(nvg, stroke_width_2_in_gui_helpers);
    float rnd = ten_in_gui_helpers;
    draw_rect_stroked(window, sw, rnd);
    float x0 = sw.x, y0 = sw.y, x1 = sw.w + sw.x;
    V15 c0 = nvg_rgb_f(col->r, col->g, col->b);
    V15 c1 = nvg_rgba_f(col->r, col->g, col->b, 0.25f);
    nvg_FillPaint(nvg, nvg_linear_gradient(nvg, x0, y0, x1, y0, c0, c1));
    draw_rect_rounded(window, sw, rnd);
    R15 cr = make_rect((float)((double)r.w * factor_0_5_in_gui_helpers_a + (double)r.x), r.y, r.w * factor_0_5_in_gui_helpers_b, r.h);
    unsigned char clicked = clicked_inside_r_frame(gui, cr);
    if (clicked) {
        Col15 c = *col;
        open_colour_picker_colour_c(gui, label, col, &c, (void *)copy_pair12);
    }
    return clicked;
}

// ---- list setting: one row per entry (name, colour swatch for colour lists, remove cross), "Click to add" ----
struct ListOps {
    void *ctx;
    unsigned long long (*count)(void *ctx);
    const char *(*name)(void *ctx, unsigned long long i);
    Col15 *(*color)(void *ctx, unsigned long long i);
    void (*add)(void *ctx, const char *name, Col15 *color);
    void (*remove)(void *ctx, unsigned long long i);
};
FN(float, draw_text_sized, (GW15 *window, R15 r, unsigned align, unsigned col, const char *text, const char *end));
FN(R15, inset_rect, (R15 r, float d));
FN(V15, nvg_trans_rgba_f, (V15 c, float k));
FN(bool, mouse_over_rect_r, (GuiState *gui, R15 r));
FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_MoveTo, (void *nvg, float x, float y));
FN(void, nvg_LineTo, (void *nvg, float x, float y));
FN(void, nvg_Stroke, (void *nvg));
FN(void, pick_add_highlight, (void *p, void *, void *key));
FN(void, pick_add_shadow, (void *p, void *key));
FN(void, open_list_popup, (GuiState *gui, void *object, void *other_object, void *c, void *d0, void *d1, void *e));
FN(void, start_timed_popup_drag_payload, (GuiState *gui, void *object, void *other_object, void *c, void *d));
extern "C" const float colour_level_0_866667_in_gui_helpers_a, font_size_15_in_gui_helpers_a, colour_alpha_0_5_in_gui_helpers_a, colour_level_0_25098_in_gui_helpers_a, stroke_width_2_in_gui_helpers_a, corner_radius_10_in_gui_helpers_a,
    inset_4, corner_radius_5_in_gui_helpers, colour_level_0_933333_in_gui_helpers, colour_level_0_345098_in_gui_helpers, gui_list_add_idle_grey_level, gui_list_add_hover_grey_level;
extern "C" const double factor_0_5_in_gui_helpers, factor_0_3_in_gui_helpers;
extern "C" const char str_click_to_add[];   // "Click to add"
extern "C" bool draw_list_editor_row(GuiState *gui, GW15 *window, const char *label, void *title, void *filter, int kind, ListOps *sel, void *items,
                         void *p9) {
    void *nvg = window->ig;
    unsigned long long n = sel->count(sel->ctx);
    R15 r = add_cell(gui, (float)(n + 27 * (n + 1) + 1));
    R15 lr = r;
    lr.h = 20.0f;
    V15 tc = nvg_rgba_f(colour_level_0_866667_in_gui_helpers_a, colour_level_0_866667_in_gui_helpers_a, colour_level_0_866667_in_gui_helpers_a, 1.0f);
    nvg_FillColor(nvg, &tc);
    nvg_FontSize(nvg, font_size_15_in_gui_helpers_a);
    draw_text(window, lr, 0, 0, label);
    R15 box = make_rect((float)((double)r.w * factor_0_5_in_gui_helpers + (double)r.x), r.y, r.w * colour_alpha_0_5_in_gui_helpers_a, r.h);
    V15 oc = nvg_rgba_f(colour_level_0_25098_in_gui_helpers_a, colour_level_0_25098_in_gui_helpers_a, colour_level_0_25098_in_gui_helpers_a, 1.0f);
    nvg_StrokeColor(nvg, &oc);
    nvg_StrokeWidth(nvg, stroke_width_2_in_gui_helpers_a);
    draw_rect_stroked(window, box, corner_radius_10_in_gui_helpers_a);
    float rh = box.h / (float)(n + 1);
    bool changed = false;
    if (n) {
        unsigned long long del = ~0ull;
        float dim = colour_level_0_25098_in_gui_helpers_a;
        for (unsigned long long i = 0; i != n; i++) {
            R15 row = make_rect(box.x, (float)i * rh + box.y, box.w, rh);
            const char *name = sel->name(sel->ctx, i);
            unsigned long long len = libc_strlen(name);
            if (len >= 0x14) len = 0x14;
            V15 nc = nvg_rgba_f(colour_level_0_866667_in_gui_helpers_a, colour_level_0_866667_in_gui_helpers_a, colour_level_0_866667_in_gui_helpers_a, 1.0f);
            nvg_FillColor(nvg, &nc);
            draw_text_sized(window, row, 1, 0, name, name + len);
            if (kind == 1) {
                R15 sw = inset_rect(R15{row.x, row.y, row.h, row.h}, inset_4);
                Col15 *col = sel->color(sel->ctx, i);
                V15 cc = nvg_rgb_f(col->r, col->g, col->b);
                nvg_FillColor(nvg, &cc);
                draw_rect_rounded(window, sw, corner_radius_5_in_gui_helpers);
                if (clicked_inside_r_frame(gui, sw)) {
                    Col15 c = *col;
                    open_colour_picker_colour_c(gui, name, col, &c, (void *)copy_pair12);
                    changed = true;
                }
            }
            float y = row.y, h = row.h, x = row.w - h + row.x;
            R15 xr = {x, y, h, h};
            V15 xc = mouse_over_rect_r(gui, xr) ? nvg_rgba_f(colour_level_0_933333_in_gui_helpers, colour_level_0_345098_in_gui_helpers, colour_level_0_345098_in_gui_helpers, 1.0f)
                                     : nvg_rgba_f(dim, dim, dim, 1.0f);
            nvg_StrokeColor(nvg, &xc);
            float in = (float)((double)h * factor_0_3_in_gui_helpers);
            nvg_BeginPath(nvg);
            float x0 = x + in, y0 = y + in, x1 = h + x - in, y1 = y + h - in;
            nvg_MoveTo(nvg, x0, y0);
            nvg_LineTo(nvg, x1, y1);
            nvg_MoveTo(nvg, x1, y0);
            nvg_LineTo(nvg, x0, y1);
            nvg_Stroke(nvg);
            if (clicked_inside_r_frame(gui, xr)) del = i;
            V15 sc = nvg_trans_rgba_f(nvg_rgba_f(dim, dim, dim, 1.0f), colour_alpha_0_5_in_gui_helpers_a);
            nvg_StrokeColor(nvg, &sc);
            nvg_BeginPath(nvg);
            nvg_MoveTo(nvg, row.x, row.y + row.h);
            nvg_LineTo(nvg, row.x + row.w, row.y + row.h);
            nvg_Stroke(nvg);
        }
        if (del != ~0ull) {
            sel->remove(sel->ctx, del);
            changed = true;
        }
    }
    R15 add = make_rect(box.x, (float)n * rh + box.y, box.w, rh);
    float k = mouse_over_rect_r(gui, add) ? gui_list_add_hover_grey_level : gui_list_add_idle_grey_level;
    V15 ac = nvg_rgba_f(k, k, k, 1.0f);
    nvg_FillColor(nvg, &ac);
    draw_text(window, add, 1, 0, str_click_to_add);
    if (clicked_inside_r_frame(gui, add)) {
        if (items) open_list_popup(gui, title, sel, filter, (void *)pick_add_highlight, items, p9);
        else start_timed_popup_drag_payload(gui, title, sel, filter, (void *)pick_add_shadow);
        changed = true;
    }
    return changed;
}
