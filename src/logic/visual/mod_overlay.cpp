// Mod Overlay - the list of enabled modules in the top right corner of the screen.
//
// Each enabled module (of the categories ticked in the settings, optionally only ones with a
// keybind) gets an entry: its name, and with "show_details" a short status ("suffix").
// Entries slide in and out, and are sorted longest first.
//
// Functions:
//   mod_overlay_on_hud_tick          HUD hook: ask for redraws while animating
//   mod_overlay_on_profile_event     a profile was loaded: rebuild
//   mod_overlay_on_overlay_unload    rebuild (keybinds may have changed)
//   mod_overlay_rebuild_visibility   show / hide entries after a settings change
//   mod_overlay_tick                 every tick: refresh names and suffixes
//   mod_overlay_module_toggled       a module was switched on / off: add / show / hide its entry
//   mod_overlay_render               every frame: animate, measure, sort and draw
//   mod_overlay_append_entry         add an entry (lowercasing is done inline here)
#include "slinky/engine.h"

struct Module {
    const char *name;
    char padding_08[8];
    int category;                   // 0 combat, 1 move, 2 visual, 3 block, 4 utility
    unsigned char enabled;
    char padding_15[3];
    int flags;                      // 4: hidden from the overlay
    int key;
    char padding_20[0x1e8];
    void (*suffix)(char *out);
};
struct ModList { Module **mods; unsigned long long count; };
struct Entry {                      // 0x50 bytes at 0x2fb000
    Module *mod;
    char text[0x14];
    char suffix[0x20];
    float w, h;
    float anim;
    unsigned char alive;
    char padding_49[7];
};
struct ToggleEvt { char padding_00[8]; Ctx *ctx; Module *mod; unsigned char on; };
struct Ui { void *dl; char padding_08[0x10]; float width; };
struct RenderEvt { char padding_00[0x10]; Ui *ui; float dt; };
struct Color { float r, g, b, a; };
struct Measure { char padding_00[8]; float w, h; };

struct FlagHook { char padding_00[0x10]; unsigned flags; };

extern "C" unsigned char mod_overlay_require_bound_value;
extern "C" unsigned char mod_overlay_dirty;
extern "C" unsigned char mod_overlay_animating;    // animating
extern "C" float mod_overlay_fade;            // fade
extern "C" unsigned mod_overlay_filter_settings_last_applied;         // filter settings last applied
extern "C" unsigned long long mod_overlay_entry_count;   // entry count
extern "C" Entry mod_overlay_entries[];
extern "C" char mod_overlay_module_descriptor[];           // Mod Overlay module descriptor
extern "C" unsigned char mod_overlay_show_per_category, mod_overlay_show_per_category_second, mod_overlay_show_per_category_third, mod_overlay_show_per_category_fourth, mod_overlay_show_per_category_fifth;   // show per category
extern "C" unsigned char mod_overlay_lowercase;    // lowercase
extern "C" unsigned char mod_overlay_show_details;    // show details
extern "C" float mod_overlay_font_scale;            // font scale
extern "C" float mod_overlay_name_colour, mod_overlay_name_colour_second, mod_overlay_name_colour_third;   // name colour
extern "C" float mod_overlay_suffix_colour, mod_overlay_suffix_colour_second, mod_overlay_suffix_colour_third;   // suffix colour
extern "C" float mod_overlay_corner_rounding;            // corner rounding
extern "C" float mod_overlay_horizontal_padding;            // horizontal padding
extern "C" float mod_overlay_vertical_padding;            // vertical padding
static const double FADE_SPEED = 5.0;     // entries fade in / out in 0.2 s
static const float FULLY_SHOWN = 1.0f;
static const float SUFFIX_GAP = 4.0f;      // pixels between the name and the suffix
S(str_s_in_mod_overlay);   // "%s"

FN(void, mod_overlay_rebuild_visibility_entry, (void *ctx));
FN(void, mod_overlay_append_entry_entry, (Module *module));
FN(unsigned long long, libc_strlen, (const char *str));
FN(void, libc_memmove, (void *dst, const void *src, unsigned long long n));
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, compare_overlay_entries_by_height_desc, (const void *left, const void *right));
FN(void, nvg_FontSize, (void *nvg, float scale));
FN(void, nvg_TextAlign, (void *nvg, int font));
FN(void, set_draw_color, (Ui *ui, int flags));
FN(void, nvg_text_bounds, (void *nvg, float size, float wrap, const char *text, void *end, Measure *out));
FN(Color, nvg_rgba_f, (float r, float g, float b, float a));
FN(Color, nvg_rgb_f, (float r, float g, float b));
FN(float, ease_curve, (float t));
FN(void, nvg_GlobalAlpha, (void *nvg, float alpha));
FN(void, nvg_FillColor, (void *nvg, Color c));
FN(void, nvg_BeginPath, (void *nvg));
FN(void, nvg_Fill, (void *nvg));
FN(void, nvg_RoundedRectVarying, (void *nvg, float x, float y, float w, float h, int a, int b, int c, float rounding));
FN(float, nvg_text, (void *nvg, float x, float y, const char *text, void *end));

static inline __attribute__((always_inline)) char lower(char ch) {
    return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch;
}

static bool category_shown(int cat) {
    switch (cat) {
    case 0: return mod_overlay_show_per_category;
    case 1: return mod_overlay_show_per_category_second;
    case 2: return mod_overlay_show_per_category_fourth;
    case 3: return mod_overlay_show_per_category_third;
    case 4: return mod_overlay_show_per_category_fifth == 1;
    default: return true;
    }
}

static bool shown(Module *module) {
    if (module->flags & 4)
        return false;
    if (mod_overlay_require_bound_value == 1 && !module->key)
        return false;
    return category_shown(module->category);
}

static Entry *find(Module *module) {
    for (unsigned long long i = 0; i < mod_overlay_entry_count; i++)
        if (mod_overlay_entries[i].mod == module)
            return &mod_overlay_entries[i];
    return nullptr;
}

extern "C" void mod_overlay_on_hud_tick(FlagHook *hook) {
    unsigned f = hook->flags;
    hook->flags = f | 1;
    if (mod_overlay_animating || mod_overlay_fade > 0.0f)
        hook->flags = f | 3;
}

extern "C" void mod_overlay_on_profile_event(FlagHook *hook) {
    if (hook->flags == 0)
        mod_overlay_dirty = 1;
}

extern "C" void mod_overlay_on_overlay_unload(Hook *hook) {
    if (mod_overlay_require_bound_value == 1) {
        mod_overlay_rebuild_visibility_entry(hook->ctx);
        mod_overlay_animating = 1;
    }
}

extern "C" void mod_overlay_rebuild_visibility(ModList *list) {
    for (unsigned long long i = 0; i < mod_overlay_entry_count; i++)
        if (!shown(mod_overlay_entries[i].mod))
            mod_overlay_entries[i].alive = 0;
    for (unsigned long long i = 0; i < list->count; i++) {
        Module *module = list->mods[i];
        if (module->enabled != 1 || !shown(module))
            continue;
        if (Entry *entry = find(module))
            entry->alive = 1;
        else
            mod_overlay_append_entry_entry(module);
    }
}

extern "C" void mod_overlay_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    unsigned mask = mod_overlay_show_per_category | mod_overlay_show_per_category_second << 1 | mod_overlay_show_per_category_third << 2 | mod_overlay_show_per_category_fourth << 3 | mod_overlay_show_per_category_fifth << 4 | mod_overlay_require_bound_value << 5;
    unsigned char dirty = mod_overlay_dirty;
    if (mod_overlay_filter_settings_last_applied != mask || (dirty & 1)) {
        mod_overlay_rebuild_visibility_entry(ctx->module_manager);
        mod_overlay_filter_settings_last_applied = mask;
        mod_overlay_animating = 1;
        mod_overlay_dirty = 0;
    }
    if (!(dirty & 1) && ctx->in_game)
        return;
    for (unsigned long long i = 0; i < mod_overlay_entry_count; i++) {
        Entry *entry = &mod_overlay_entries[i];
        Module *module = entry->mod;
        if (mod_overlay_lowercase & 1) {
            unsigned long long n = libc_strlen(module->name);
            for (unsigned long long j = 0; j < n; j++)
                entry->text[j] = lower(module->name[j]);
            entry->text[n] = 0;
        } else {
            log_fmt(entry->text, 0x14, str_s_in_mod_overlay, module->name);
        }
        if (!module->suffix || !mod_overlay_show_details) {
            entry->suffix[0] = 0;
            continue;
        }
        module->suffix(entry->suffix);
        if (mod_overlay_lowercase == 1)
            for (unsigned long long j = 0; j < libc_strlen(entry->suffix); j++)
                entry->suffix[j] = lower(entry->suffix[j]);
    }
    mod_overlay_animating = 1;
}

extern "C" void mod_overlay_module_toggled(ToggleEvt *h) {
    Module *module = h->mod;
    mod_overlay_fade = 0.2f;
    if ((void *)module == mod_overlay_module_descriptor) {             // the overlay itself: rebuild from scratch
        ModList *list = (ModList *)h->ctx->module_manager;
        mod_overlay_entry_count = 0;
        if (h->on != 1)
            return;
        for (unsigned long long i = 0; i < list->count; i++) {
            Module *o = list->mods[i];
            if (o->enabled == 1 && shown(o))
                mod_overlay_append_entry_entry(o);
        }
        mod_overlay_animating = 1;
        return;
    }
    if (Entry *entry = find(module)) {
        if (!h->on)
            entry->alive = 0;
        else if (shown(module))
            entry->alive = 1;
        return;
    }
    if (h->on != 1 || !shown(module))
        return;
    mod_overlay_append_entry_entry(module);
    mod_overlay_animating = 1;
}

extern "C" void mod_overlay_render(RenderEvt *h) {
    Ui *window = h->ui;
    void *nvg = window->dl;
    nvg_FontSize(nvg, mod_overlay_font_scale);
    set_draw_color(window, 0);
    if (mod_overlay_fade > 0.0f)
        mod_overlay_fade -= h->dt;
    double up = (double)h->dt * FADE_SPEED;
    double down = -(double)h->dt * FADE_SPEED;
    unsigned long long n = mod_overlay_entry_count;
    for (unsigned long long i = 0; i < n; i++) {
        Entry *entry = &mod_overlay_entries[i];
        if (entry->alive == 1) {
            if (entry->anim != FULLY_SHOWN) {
                float a = (float)(entry->anim + up);
                entry->anim = FULLY_SHOWN < a ? FULLY_SHOWN : a;
            }
            continue;
        }
        float a = (float)(entry->anim + down);

        entry->anim = a;
        if (a > 0.0f)
            continue;
        mod_overlay_entry_count = n - 1;                    // faded out: drop the entry
        libc_memmove(entry, entry + 1, (n - 1 - i) * sizeof(Entry));
        n -= 1;
    }
    if (mod_overlay_animating == 1) {
        nvg_TextAlign(nvg, 9);
        Measure m;
        for (unsigned long long i = 0; i < mod_overlay_entry_count; i++) {
            Entry *e = &mod_overlay_entries[i];
            nvg_text_bounds(nvg, 0.0f, 0.0f, e->text, nullptr, &m);
            e->w = m.w;
            e->h = m.h;
            if (!e->suffix[0])
                continue;
            nvg_text_bounds(nvg, 0.0f, 0.0f, e->suffix, nullptr, &m);
            e->w = m.w + SUFFIX_GAP + e->w;
        }
        libc_qsort(mod_overlay_entries, mod_overlay_entry_count, sizeof(Entry), compare_overlay_entries_by_height_desc);
        mod_overlay_animating = 0;
    }
    nvg_TextAlign(nvg, 9);
    Color bg = nvg_rgba_f(0.0f, 0.0f, 0.0f, 0.7f);
    Color name = nvg_rgb_f(mod_overlay_name_colour, mod_overlay_name_colour_second, mod_overlay_name_colour_third);
    Color detail = nvg_rgb_f(mod_overlay_suffix_colour, mod_overlay_suffix_colour_second, mod_overlay_suffix_colour_third);
    float width = window->width;
    float y = 0.0f;
    for (unsigned long long i = 0; i < mod_overlay_entry_count; i++) {
        Entry *e = &mod_overlay_entries[i];
        float alpha = e->anim == FULLY_SHOWN ? FULLY_SHOWN : ease_curve(e->anim);
        nvg_GlobalAlpha(nvg, alpha);
        float round = mod_overlay_corner_rounding;
        if (i + 1 < mod_overlay_entry_count) {
            float d = e->w - mod_overlay_entries[i + 1].w;
            round = d < round ? d : round;
        }
        nvg_FillColor(nvg, bg);
        nvg_BeginPath(nvg);
        float pad = mod_overlay_horizontal_padding + mod_overlay_horizontal_padding;
        nvg_RoundedRectVarying(nvg, width - pad - e->w * alpha, y, pad + e->w, mod_overlay_vertical_padding + mod_overlay_vertical_padding + e->h, 0, 0, 0, round);
        nvg_Fill(nvg);
        nvg_FillColor(nvg, name);
        float ty = mod_overlay_vertical_padding + y;
        float end = nvg_text(nvg, width - mod_overlay_horizontal_padding - e->w * alpha, ty, e->text, nullptr);
        if (e->suffix[0]) {
            nvg_FillColor(nvg, detail);
            nvg_text(nvg, end + SUFFIX_GAP, ty, e->suffix, nullptr);
        }
        y += alpha * (mod_overlay_vertical_padding + mod_overlay_vertical_padding + e->h);
    }
    nvg_GlobalAlpha(nvg, FULLY_SHOWN);
}

extern "C" void mod_overlay_append_entry(Module *module) {
    if (mod_overlay_entry_count >= 50)                // mod_overlay_entries is exactly 50 * sizeof(Entry) bytes
        return;
    Entry *entry = &mod_overlay_entries[mod_overlay_entry_count];
    entry->mod = module;
    entry->anim = 0.0f;
    entry->alive = 1;
    if (mod_overlay_lowercase) {
        unsigned long long n = libc_strlen(module->name);
        if (n > 0x13) n = 0x13;           // text is 0x14 bytes including the terminator
        for (unsigned long long j = 0; j < n; j++)
            entry->text[j] = lower(module->name[j]);
        entry->text[n] = 0;
    } else {
        log_fmt(entry->text, 0x14, str_s_in_mod_overlay, module->name);
        module = entry->mod;
    }
    if (module->suffix && mod_overlay_show_details) {
        module->suffix(entry->suffix);
        if (mod_overlay_lowercase == 1 && entry->suffix[0])
            for (unsigned long long j = 0; j < libc_strlen(entry->suffix); j++)
                entry->suffix[j] = lower(entry->suffix[j]);
    } else {
        entry->suffix[0] = 0;
    }
    mod_overlay_entry_count += 1;
}
