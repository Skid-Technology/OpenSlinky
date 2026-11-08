// GUI: module cards - header, settings body, per-card open/close animation.
#include "slinky/engine.h"
#include "slinky/gui_state.h"
FN(void, close_popup, (void *x));
extern "C" void close_popup_now(void *x) { close_popup(x); }
extern "C" void reset_drag_state(GuiState *gui) { gui->input_mode = 0; gui->click_capture = 0; }

struct R16f { float x, y, w, h; };
struct V4f { float x, y, z, w; };
FN(float, libc_powf, (float x, float y));   // powf
FN(void, nvg_GlobalAlpha, (void *nvg, float alpha));
FN(V4f, nvg_rgba_f, (float r, float g, float b, float a));
FN(V4f, nvg_trans_rgba_f, (V4f c, float k));
FN(void, nvg_FillColor, (void *nvg, V4f *c));
FN(R16f, make_rect, (float x, float y, float w, float h));
FN(void, draw_rect_filled, (void *window, R16f *r));
FN(void, nvg_Translate, (void *nvg, float a, float b));
extern "C" const float one_in_gui_cards, five_in_gui_cards, gui_card_header_grey_level, colour_alpha_0_75, gui_card_slide_offset, factor_50;
struct WinA { void *ig; char padding_08[0x10]; float w, h; };
// open/close animation: ease-out alpha, dimmed backdrop, slide offset
extern "C" void card_open_animation(WinA *window, float t) {
    float one = one_in_gui_cards;
    float alpha = one - libc_powf(one - t, five_in_gui_cards);
    void *nvg = window->ig;
    nvg_GlobalAlpha(nvg, alpha);
    float k = gui_card_header_grey_level;
    V4f c = nvg_rgba_f(k, k, k, 1.0f);
    V4f c2 = nvg_trans_rgba_f(c, colour_alpha_0_75);
    nvg_FillColor(nvg, &c2);
    R16f r = make_rect(0.0f, 0.0f, window->w, window->h);
    draw_rect_filled(window, &r);
    nvg_Translate(nvg, 0.0f, gui_card_slide_offset);
    nvg_Translate(nvg, 0.0f, alpha * factor_50);
}

// ---- module cards of one category: header (name, bind button, toggle), description, settings ----
struct Reg2 { void **list; unsigned long long n; };
struct Mgr2 { void *mods; unsigned long long count; Reg2 cat[5]; char padding_60[0x308 - 0x60]; void *set308; };
struct ModC { const char *name; const char *desc; char padding_10[4]; unsigned char enabled; char padding_15[3]; unsigned flags;
              int bind; unsigned char hold; char padding_21[7]; char anim[0x10]; float height; char padding_3c[4];
              unsigned char expanded; char padding_41[0x1b7]; void (*settings)(GuiState *gui, void *w); };
FN(void, choose_columns, (GuiState *gui, float w));
FN(R16f, add_cell, (GuiState *gui, float h));
FN(bool, rects_intersect, (const R16f *a, const R16f *b));
FN(void, draw_rect_rounded, (void *window, const R16f *r, float round));
FN(void, nvg_Scissor, (void *nvg, float x, float y, float w, float h));
FN(void, begin_content, (void *window, GuiState *gui, R16f *r, float pad, float f));
FN(void, pop_layout_row, (GuiState *gui, int n));
FN(float, draw_module_header, (GuiState *gui, void *object, void *other_object, const R16f *r, float fsz, const V4f *col, const V4f *hcol, const char *text));
FN(void, nvg_FontSize, (void *nvg, float size));
FN(float, draw_text, (void *window, R16f r, int align, int col, const char *text));
FN(void, draw_module_description, (GuiState *gui, void *object, void *cbArg, void *target, int key, unsigned char *hold, const R16f *r));
FN(bool, draw_animated_description, (GuiState *gui, void *object, R16f *r, void *anim, unsigned char *val));
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));
FN(float, end_layout_row, (void *window, GuiState *gui));
FN(float, lerp_clamped_float, (float a, float b, float t));
extern "C" const float gui_cards_column_width, gui_card_background_grey_level, row_height_20_in_gui_cards, content_padding_8_in_gui_cards, font_size_16_in_gui_cards, colour_level_0_866667_in_gui_cards, colour_level_1_in_gui_cards_a,
    rect_extent_5_in_gui_cards_a, rect_extent_neg_40, rect_extent_40, colour_level_0_6_in_gui_cards, colour_level_0_733333, offset_9_22337e18;
extern "C" const double divisor_0_05_in_gui_cards;
static inline __attribute__((always_inline)) V4f grey12(float k) { return nvg_rgba_f(k, k, k, 1.0f); }
extern "C" void draw_module_cards(GuiState *gui, WinA *window, Mgr2 *mgr, unsigned cat, float dt) {
    void *nvg = window->ig;
    choose_columns(gui, gui_cards_column_width);
    Reg2 *list = &mgr->cat[cat];
    if (!list->n) return;
    float speed = (float)((double)dt / divisor_0_05_in_gui_cards);
    for (unsigned long long i = 0; i < list->n; i++) {
        ModC *module = (ModC *)list->list[i];
        R16f card = add_cell(gui, module->height);
        // the content height grows through the per-card end_layout_row finalizers alone, exactly like the
        // original: a card outside the viewport is only measured once it scrolls into view
        R16f view = make_rect(0.0f, gui->scroll, window->w, window->h);
        if (!rects_intersect(&view, &card)) continue;
        V4f cc;
        volatile float ck = gui_card_background_grey_level;
        cc.x = cc.y = cc.z = ck;
        cc.w = 1.0f;
        nvg_FillColor(nvg, &cc);
        draw_rect_rounded(window, &card, row_height_20_in_gui_cards);
        R16f inner = card;
        begin_content(window, gui, &inner, content_padding_8_in_gui_cards, 12.0f);
        pop_layout_row(gui, 1);
        void *ig2 = window->ig;
        R16f hdr = add_cell(gui, row_height_20_in_gui_cards);
        float x;
        if (gui->card_layout_mode == 1) {
            V4f c1 = grey12(colour_level_0_866667_in_gui_cards), c2 = grey12(colour_level_1_in_gui_cards_a);
            x = draw_module_header(gui, window, module, &hdr, 16.0f, &c1, &c2, module->name);
        } else {
            nvg_FontSize(ig2, font_size_16_in_gui_cards);
            V4f c = grey12(colour_level_0_866667_in_gui_cards);
            nvg_FillColor(ig2, &c);
            x = draw_text(window, hdr, 0, 0, module->name);
        }
        unsigned f = module->flags;
        if (!(f & 1)) {
            R16f kb = make_rect(x + rect_extent_5_in_gui_cards_a, hdr.y, hdr.w - x + hdr.x, hdr.h);
            draw_module_description(gui, window, mgr->set308, module, module->bind, (module->flags & 8) ? 0 : &module->hold, &kb);
            f = module->flags;
        }
        if (!(f & 2)) {
            R16f tg = make_rect(hdr.x + hdr.w + rect_extent_neg_40, hdr.y, rect_extent_40, hdr.h);
            unsigned char on = module->enabled;
            draw_animated_description(gui, window, &tg, module->anim, &on);
            if (on != module->enabled) queue_module_toggle(mgr, module, on);
        }
        if (gui->card_layout_mode == 0) {
            R16f d = add_cell(gui, row_height_20_in_gui_cards);
            V4f c1 = grey12(colour_level_0_6_in_gui_cards), c2 = grey12(colour_level_0_733333);
            draw_module_header(gui, window, module, &d, 14.0f, &c1, &c2, module->desc);
        }
        if (module->expanded == 1 && module->settings) module->settings(gui, window);
        float used = end_layout_row(window, gui);
        long long ia = (long long)used;
        long long ib = (long long)(used - offset_9_22337e18);   // float -> uint64 like the original (no constant pool)
        unsigned long long u = (unsigned long long)((ib & (ia >> 63)) | ia);
        float target = (float)u;
        if (module->height != 0.0f) target = lerp_clamped_float(module->height, target, speed);
        module->height = target;
    }
}
extern "C" unsigned char is_unload_requested(GuiState *gui) { return (unsigned char)gui->unload_requested; }

// is rect r inside the scrolled viewport (0, scroll, width, height)?
extern "C" bool rect_in_viewport(GuiState *gui, WinA *w, const R16f *r) {
    R16f view = make_rect(0.0f, gui->scroll, w->w, w->h);
    R16f rr = *r;
    return rects_intersect(&view, &rr);
}
