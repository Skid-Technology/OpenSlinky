// 0x10000: GUI window helpers
#include "slinky/engine.h"
#include "slinky/gui_state.h"
FN(void, nvg_FontFaceId, (void *imgui, unsigned col));
struct GCol { void *imgui; char padding_08[0x38]; unsigned col[1]; };
extern "C" void set_draw_color(GCol *g, unsigned i) {
    void *nvg = g->imgui;

    nvg_FontFaceId(nvg, g->col[i]);
}
extern "C" void reset_layout_columns(GuiState *gui) { gui->layout_row_depth = 0; gui->layout_column_count = 0; }

FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_Rect, (void *nvg, float x, float y, float w, float h));
FN(void, nvg_Fill, (void *nvg));
extern "C" void draw_rect_filled(void **w, const float *r) {
    void *nvg = w[0];
    nvg_BeginPath(nvg);
    float x = r[0];
    float y = r[1];
    float w2 = r[2];
    float h = r[3];
    nvg_Rect(nvg, x, y, w2, h);
    nvg_Fill(nvg);
}

FN(void, nvg_RoundedRect, (void *nvg, float x, float y, float w, float h, float round));
FN(void, nvg_Stroke, (void *nvg));
extern "C" void draw_rect_rounded(void **w, const float *r, float round) {
    float x = r[0], y = r[1], ww = r[2], h = r[3];
    void *nvg = w[0];
    nvg_BeginPath(nvg);
    nvg_RoundedRect(nvg, x, y, ww, h, round);
    nvg_Fill(nvg);
}
extern "C" void draw_rect_stroked(void **w, const float *r, float round) {
    float x = r[0], y = r[1], ww = r[2], h = r[3];
    void *nvg = w[0];
    nvg_BeginPath(nvg);
    nvg_RoundedRect(nvg, x, y, ww, h, round);
    nvg_Stroke(nvg);
}

struct R16e { float x, y, w, h; };
struct V4e { float x, y, z, w; };
FN(void, push_layout_row, (GuiState *gui, R16e *r, float sp, float pad));
FN(R16e, round_rect, (R16e r));
FN(void, nvg_Save, (void *nvg));
FN(void, nvg_Scissor, (void *nvg, float x, float y, float w, float h));
typedef int v4ie __attribute__((vector_size(16)));
typedef float v4fe __attribute__((vector_size(16)));
extern "C" const v4ie gui_layout_sign_mask;   // sign mask
// begin a child region: push it, round the rect to whole pixels and clip to its padded inside
extern "C" void begin_content(void **w, GuiState *gui, R16e *r, float pad, float f) {
    void *nvg = w[0];
    // the original round-trips a stack copy of the rect: push_layout_row clips and insets the copy to build
    // the pushed row while the caller's rect stays raw, then the raw rect is rounded and reused for
    // the scissor - whose left/top edge sits at the card edge, not at the already-inset row. Scissoring
    // the inset row shaved the first glyph off every label ("mode" -> "ode").
    R16e local = *r;
    push_layout_row(gui, &local, pad, f);
    gui->rows[gui->layout_row_depth - 1].clip = 1;
    *r = round_rect(*r);
    nvg_Save(nvg);
    v4fe v = {pad, 0, 0, 0};
    float np = ((v4fe)((v4ie)v ^ gui_layout_sign_mask))[0] - pad;
    nvg_Scissor(nvg, r->x + pad, r->y + pad, r->w + np, np + r->h);
}
FN(float, libc_sinf, (float x));   // sinf
FN(V4e, nvg_trans_rgba_f, (V4e c, float k));
FN(void, nvg_StrokeColor, (void *nvg, V4e *c));
FN(void, nvg_StrokeWidth, (void *nvg, float w));
FN(void, nvg_MoveTo, (void *nvg, float x, float y));
FN(void, nvg_LineTo, (void *nvg, float x, float y));
extern "C" const float half_constant;
struct WT { void *ig; double t; };
// pulsing vertical accent line at the left edge of r
extern "C" void draw_accent_line(WT *w, R16e *r, V4e *col, float speed, float width) {
    void *nvg = w->ig;
    float s = libc_sinf((float)((double)speed * w->t));
    float k = half_constant;
    k = s * k + k;
    nvg_BeginPath(nvg);
    V4e c = nvg_trans_rgba_f(*col, k);
    nvg_StrokeColor(nvg, &c);
    nvg_StrokeWidth(nvg, width);
    nvg_MoveTo(nvg, r->x, r->y);
    nvg_LineTo(nvg, r->x, r->y + r->h);
    nvg_Stroke(nvg);
}

extern "C" const float minus_one_in_gui_layout_a;
// set the column count of the current layout row (finishing the previous row's height)
typedef GuiCell Cell;
typedef GuiLayoutRow Row;
extern "C" void pop_layout_row(GuiState *gui, int n) {
    Row *row = &gui->rows[gui->layout_row_depth - 1];
    unsigned long long k = row->items;
    if (k) {
        k--;
        // the compiled binary here reloads only one cell, (items-1)&3 i.e. the last added one: its
        // "max over the last visual row" loop re-maxes that same value into a register constant and
        // moves it - dead straight-lining. Two columns therefore never cross-balance their bottoms.
        Cell *c = &row->cell[k & 3];
        float m = c->y + c->h;
        m = m > minus_one_in_gui_layout_a ? m : minus_one_in_gui_layout_a;
        if (m != minus_one_in_gui_layout_a) row->bottom = m + row->y0;
    }
    int cols = n < 4 ? n : 4;
    if (!n) cols = 1;
    row->cols = cols;
    row->items = 0;
}
FN(void, nvg_TextAlign, (void *nvg, unsigned font));
FN(float, nvg_text, (void *nvg, float x, float y, void *text, void *end));
extern "C" const unsigned text_align_flags_table[];
extern "C" const float offset_1_in_gui_layout_c;
extern "C" const double factor_0_5_in_gui_layout_a;   // 0.5
// text in rect r: align 0 = left, 1 = centre, 2 = right; vertically centred
extern "C" void draw_text_sized(GCol *window, R16e *r, unsigned align, unsigned col, void *text, void *end) {
    set_draw_color(window, col);
    void *nvg = window->imgui;
    nvg_TextAlign(nvg, text_align_flags_table[align]);
    float x = r->x;
    float y = r->y + offset_1_in_gui_layout_c;
    r->y = y;
    if (align != 0) {
        if (align == 2) x = x + r->w;
        else x = (float)((double)r->w * factor_0_5_in_gui_layout_a + (double)x);
    }
    float yy = (float)((double)r->h * factor_0_5_in_gui_layout_a + (double)y);
    nvg_text(nvg, x, yy, text, end);
}

extern "C" const float minus_one_in_gui_layout_b;
// pick the column count so that cells of width w fit the row
extern "C" void choose_columns(GuiState *gui, float w) {
    Row *row = &gui->rows[gui->layout_row_depth - 1];
    float avail = row->width, sp = row->y0;
    unsigned long long k = row->items;
    if (k) {
        k--;
        // same single-cell finalizer as end_layout_row/pop_layout_row, see the note there
        Cell *c = &row->cell[k & 3];
        float m = c->y + c->h;
        m = m > minus_one_in_gui_layout_b ? m : minus_one_in_gui_layout_b;
        if (m != minus_one_in_gui_layout_b) row->bottom = m + sp;
    }
    int n = (int)((avail + sp) / (w + sp));
    int cols = n < 4 ? n : 4;
    if (!n) cols = 1;
    row->cols = cols;
    row->items = 0;
}
extern "C" const float one_in_gui_layout_b;
extern "C" const double factor_0_5_in_gui_layout;
// text in rect r (read-only), returns its width
extern "C" float draw_text(GCol *window, const R16e *r, unsigned align, unsigned col, void *text) {
    float x = r->x, y = r->y, w = r->w, h = r->h;
    set_draw_color(window, col);
    void *nvg = window->imgui;
    nvg_TextAlign(nvg, text_align_flags_table[align]);
    if (align != 0) {
        if (align == 2) x = x + w;
        else x = (float)((double)w * factor_0_5_in_gui_layout + (double)x);
    }
    y += one_in_gui_layout_b;
    float yy = (float)((double)h * factor_0_5_in_gui_layout + (double)y);
    return nvg_text(nvg, x, yy, text, 0);
}

extern "C" const float minus_one_in_gui_layout;
FN(void, nvg_Restore, (void *nvg));
// end the current layout row: finish its height, pop its clip, grow the window height; returns the used height
extern "C" float end_layout_row(GCol *w, GuiState *gui) {
    Row *row = &gui->rows[gui->layout_row_depth - 1];
    unsigned long long k = row->items;
    if (k) {
        k--;
        // only the last added cell ((items-1)&3) reaches the finalizer in the compiled original
        Cell *c = &row->cell[k & 3];
        float m = c->y + c->h;
        m = m > minus_one_in_gui_layout ? m : minus_one_in_gui_layout;
        if (m != minus_one_in_gui_layout) row->bottom = m + row->y0;
    }
    if (row->clip == 1) nvg_Restore(w->imgui);
    float bottom = row->bottom;
    if (bottom > (float)gui->layout_column_count) gui->layout_column_count = (int)bottom;
    gui->layout_row_depth -= 1;
    return (row->y0 + row->y0) + (bottom - row->top);
}

FN(R16e, inset_rect, (R16e r, float d));
FN(unsigned long long, offset_rect, (R16e r));
// push a layout row: clip r to the parent row, inset it by pad, start the cursor at its corner
extern "C" void push_layout_row(GuiState *gui, R16e *r, float sp, float pad) {
    long long n = gui->layout_row_depth;
    if (n < 0) n = 0;
    if (n >= 5) n = 4;   // rows[5] is the fixed stack size
    if (n) {
        float *p = (float *)&gui->rows[n - 1];
        float px = p[0], py = p[1], pw = p[2], ph = p[3];
        float x = r->x;
        if (px > x) { r->x = px; x = px; }
        float y = r->y;
        if (py > y) { r->y = py; y = py; }
        float right = px + pw;
        if (r->w + x > right) {
            r->w = right - x;
            if (0.0f > r->w) r->w = 0.0f;
        }
        float bottom = py + ph;
        if (r->h + y > bottom) {
            r->h = bottom - y;
            if (0.0f > r->h) r->h = 0.0f;
        }
    }
    char *row = (char *)&gui->rows[n];
    gui->layout_row_depth = n + 1;
    *r = inset_rect(*r, pad);
    __builtin_memset(row, 0, 0x78);
    *(R16e *)row = *r;
    *(float *)(row + 0x10) = sp;
    *(float *)(row + 0x14) = pad;
    *(unsigned long long *)(row + 0x1c) = offset_rect(*r);
}
struct Img { char b[0x4c]; };
struct Off2 { float x, y; };
FN(Img, nvg_image_pattern, (void *nvg, float x, float y, float w, float h, int z, int tex, float f));
FN(void, nvg_FillPaint, (void *nvg, Img im));
extern "C" const float twenty;
extern "C" Off2 *ptr_to_unreferenced_rodata;
struct GImg { void *imgui; char padding_08[0x40]; int tex; float tw, th; };
// draw texture tile `idx` scaled into r, clipped to r
extern "C" void draw_texture(GImg *w, R16e *r, unsigned idx, float alpha) {
    void *nvg = w->imgui;
    float k = twenty;
    float sx = r->w / k, sy = r->h / k;
    Off2 *t = ptr_to_unreferenced_rodata;
    float ox = r->x - t[idx].x * sx;
    float oy = r->y - t[idx].y * sy;
    Img im = nvg_image_pattern(nvg, ox, oy, sx * w->tw, sy * w->th, 0, w->tex, alpha);
    nvg_BeginPath(nvg);
    nvg_Rect(nvg, r->x, r->y, r->w, r->h);
    nvg_FillPaint(nvg, im);
    nvg_Fill(nvg);
}

// next cell of the current layout row (height h), rounded to whole pixels
extern "C" R16e add_cell(GuiState *gui, float h) {
    long long top = gui->layout_row_depth;
    if (top <= 0) return R16e{0.0f, 0.0f, 0.0f, h};
    Row *row = &gui->rows[top - 1];
    unsigned long long cols = (unsigned long long)row->cols;
    if (!cols) {
        row->cols = 1;
        cols = 1;
    }
    unsigned long long n = row->items;
    row->items = n + 1;
    Cell *above = &row->cell[(n - cols) & 3];
    Cell *c = &row->cell[n & 3];
    if (n % cols) {
        Cell *prev = &row->cell[(n - 1) & 3];
        float pw = prev->w;
        c->x = prev->x + pw + row->y0;
        c->w = pw;
        c->y = n < cols ? row->bottom : above->y + above->h + row->y0;
    } else {
        c->x = *(float *)((char *)row + 0x1c);
        if (n < cols) {
            c->y = row->bottom;
            c->w = (row->width - (float)(cols - 1) * row->y0) / (float)cols;
        } else {
            c->y = above->y + above->h + row->y0;
            c->w = above->w;
        }
    }
    c->h = h;
    return round_rect(*(R16e *)c);
}

// ---- input events: 1 mouse move, 2 key/button (+ up to 4 typed chars), 3 scroll; returns "consumed" ----
FN(bool, popup_is_open, (GuiState *gui));
FN(unsigned long long, libc_strlen, (const char *str));
FN(void, close_popup, (GuiState *gui));
extern "C" const float factor_neg_30;
struct InEv { int type; union { int key; float wheel; }; union { int y; unsigned char ch[4]; }; unsigned char down; };
typedef void (*CharFilter)(char *buf, char c, void *arg);
extern "C" bool handle_gui_input(GuiState *gui, InEv *in) {
    int st;
    switch (in->type) {
    case 1:
        gui->mouse_raw.x = (float)in->key;
        gui->mouse_raw.y = (float)in->y;
        return false;
    case 3:
        if (!popup_is_open(gui)) gui->scroll += in->wheel * factor_neg_30;
        return false;
    case 2:
        if (in->down != 1) break;
        st = gui->input_mode;
        if ((unsigned)(st - 2) < 2) {           // text field active
            int k = in->key;
            if (k > 0x54) {
                if (k == 0x55) {                 // enter
                    if (gui->edit_text[0]) gui->edit_confirmed = 1;
                    return true;
                }
                if (k == 0x57) {                 // escape
                    gui->input_mode = 0;
                    return true;
                }
            } else if (k == 1) {
                if (st != 3) {
                    gui->input_mode = 0;
                    return true;
                }
                break;
            } else if (k == 0x53) {              // backspace
                unsigned long long n = libc_strlen(gui->edit_text);
                if (n) gui->edit_text[n - 1] = 0;
                return true;
            }
            CharFilter edit_char_filter = ((CharFilter)gui->edit_char_filter);
            for (int i = 0; i < 4; i++) {
                char c = (char)in->ch[i];
                if (!c) return true;
                edit_char_filter = ((CharFilter)gui->edit_char_filter);
                edit_char_filter(gui->edit_text, c, gui->edit_filter_arg);
            }
            return true;
        }
        if (st == 1) {                           // key-bind capture
            int k = in->key;
            int key = (k == 1 || k == 0x57) ? 0 : k;
            ((void (*)(void *, int))gui->keybind_callback)(gui->keybind_target, key);
            gui->input_mode = 0;
            return true;
        }
        if (st == 0 && popup_is_open(gui) && in->key == 0x57) {
            close_popup(gui);
            return true;
        }
        break;
    default:
        return false;
    }
    {
        int k = in->key;
        if ((unsigned)(k - 100) < 2) gui->numeric_drag_enabled = in->down;
        else if (k == 1) gui->click_capture = in->down;
    }
    return false;
}

// ---- GUI render target: ImGui frame into an offscreen framebuffer scaled by `scale` ----
struct FBuf { char padding_00[0x14]; int tex; };
struct Rend { void *ig; double t; int w, h; float sw, sh; float scale; int padding_24; FBuf *fb; float clear[4]; };
FN(void, nvg_wrap_delete_framebuffer, (FBuf *fb));                                    // destroy
FN(FBuf *, nvglu_create_framebuffer, (void *nvg, int w, int h, int flags));         // create
FN(void, nvg_wrap_bind_framebuffer, (FBuf *fb));                                    // bind (0 = default)
FN(void, nvg_BeginFrame, (void *nvg, float w, float h, float dpi));       // begin frame
FN(void, nvg_EndFrame, (void *nvg));                                    // end frame
FN(void, nvg_Scale, (void *nvg, float x, float y));
extern "C" void (**const ptr_to_pGlGetFloatv)(unsigned pname, float *out);   // glGetFloatv
extern "C" void (**const ptr_to_pGlClearColor)(float r, float g, float b, float a);   // glClearColor
extern "C" void (**const ptr_to_pGlClear)(unsigned mask);   // glClear
extern "C" float frame_ratio_1_in_gui_layout, frame_ratio_1_in_gui_layout_a;
extern "C" void set_render_scale(Rend *renderer, float scale) {
    renderer->scale = scale;
    renderer->sw = (float)renderer->w / scale;
    renderer->sh = (float)renderer->h / scale;
}
// (re)create the framebuffer when the window size changed
extern "C" bool resize_render_target(Rend *renderer, int w, int h) {
    bool changed;
    if (w == renderer->w && h == renderer->h) {
        changed = false;
    } else {
        if (renderer->fb) nvg_wrap_delete_framebuffer(renderer->fb);
        renderer->fb = nvglu_create_framebuffer(renderer->ig, w, h, 0);
        changed = true;
    }
    renderer->w = w;
    renderer->h = h;
    return changed;
}
extern "C" void begin_frame(Rend *renderer, double t) {
    float fw = (float)renderer->w;

    void *nvg = renderer->ig;
    float fh = (float)renderer->h;

    nvg_BeginFrame(nvg, fw, fh, frame_ratio_1_in_gui_layout);
    nvg_Scale(renderer->ig, renderer->scale, renderer->scale);
    renderer->t = t;
}
// begin a frame into the framebuffer (saves and clears the GL clear colour)
extern "C" void begin_frame_framebuffer(Rend *renderer, double t) {
    nvg_wrap_bind_framebuffer(renderer->fb);
    (*ptr_to_pGlGetFloatv)(0xc22, renderer->clear);   // GL_COLOR_CLEAR_VALUE
    (*ptr_to_pGlClearColor)(0.0f, 0.0f, 0.0f, 0.0f);
    (*ptr_to_pGlClear)(0x4400);            // colour | stencil
    float fw = (float)renderer->w;

    void *nvg = renderer->ig;
    float fh = (float)renderer->h;

    nvg_BeginFrame(nvg, fw, fh, frame_ratio_1_in_gui_layout_a);
    nvg_Scale(renderer->ig, renderer->scale, renderer->scale);
    renderer->t = t;
}
// draw the framebuffer texture over the screen
extern "C" void draw_framebuffer_texture_over_screen(Rend *renderer) {
    void *nvg = renderer->ig;
    float sw = renderer->sw, sh = renderer->sh;

    Img g = nvg_image_pattern(nvg, 0.0f, 0.0f, sw, sh, 0, renderer->fb->tex, 1.0f);
    nvg_BeginPath(nvg);
    nvg_Rect(nvg, 0.0f, 0.0f, renderer->sw, renderer->sh);
    nvg_FillPaint(nvg, g);
    nvg_Fill(nvg);
}
extern "C" void end_frame(Rend *renderer) { nvg_EndFrame(renderer->ig); }
extern "C" void clear_frame(Rend *renderer) {
    nvg_EndFrame(renderer->ig);
    nvg_wrap_bind_framebuffer(0);
    void (*cc)(float, float, float, float) = *ptr_to_pGlClearColor;

    float c0 = renderer->clear[0], c1 = renderer->clear[1], c2 = renderer->clear[2], c3 = renderer->clear[3];

    cc(c0, c1, c2, c3);
}
