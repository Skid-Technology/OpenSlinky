// 0xc000: named profiles (non-global modules) load/save via cJSON
#include "slinky/engine.h"
struct cJSON;
struct Mod { char padding_00[0x18]; unsigned char flags; };
struct Mods { Mod **data; unsigned long long count; };
FN(cJSON *, cjson_parse, (const char *text));     // cJSON_Parse
FN(void, cjson_delete, (cJSON *object));             // cJSON_Delete
FN(void, cjson_free_string, (void *p));              // cJSON_free
FN(cJSON *, cjson_create_object, ());                  // cJSON_CreateObject
FN(char *, cjson_print_unformatted, (const cJSON *object));     // cJSON_PrintUnformatted
FN(unsigned char, load_module_settings, (cJSON *root, Mods *mods, Mod *module));
FN(unsigned char, save_module_settings, (cJSON *root, Mod *module));
FN(int, settings_path, (char **path));
FN(int, settings_path_append, (char **path, const char *suffix));
FN(int, settings_path_ensure, (char **path));
FN(void, free_path, (char **path));
FN(int, write_text_file, (char **path, const char *data, bool text));
FN(void, set_saved_string, (const char *s));
extern "C" const char str_slinky_in_profiles[], str_profiles[];
extern "C" unsigned char settings_dirty_flag;

extern "C" bool load_module_states(const char *text, Mods *mods) {
    cJSON *root = cjson_parse(text);
    if (!root) return false;
    bool ok = true;
    for (unsigned long long i = 0; i < mods->count; i++) {
        Mod *module = mods->data[i];
        if (module->flags & 0x10) continue;
        if (!load_module_settings(root, mods, module)) { ok = false; break; }
    }
    cjson_delete(root);
    return ok;
}
extern "C" char *serialize_module_states(Mods *mods) {
    cJSON *root = cjson_create_object();
    char *out = 0;
    if (!root) goto done;
    for (unsigned long long i = 0; i < mods->count; i++) {
        Mod *module = mods->data[i];
        if (module->flags & 0x10) continue;
        if (!save_module_settings(root, module)) goto done;
    }
    out = cjson_print_unformatted(root);
done:
    cjson_delete(root);
    return out;
}
extern "C" int save_profile(const char *name, bool text, Mods *mods) {
    char *path;
    int error_code = settings_path(&path);
    if (error_code) return error_code;
    if ((error_code = settings_path_append(&path, str_slinky_in_profiles)) != 0) goto clean;
    if ((error_code = settings_path_ensure(&path)) != 0) goto clean;
    if ((error_code = settings_path_append(&path, str_profiles)) != 0) goto clean;
    if ((error_code = settings_path_ensure(&path)) != 0) goto clean;
    if ((error_code = settings_path_append(&path, name)) != 0) goto clean;
    {
        char *json = serialize_module_states(mods);
        if (!json) {
            error_code = 0x1388;
        } else {
            error_code = write_text_file(&path, json, text);
            if (!error_code) {
                set_saved_string(name);
                settings_dirty_flag = 0;
            }
            cjson_free_string(json);
        }
    }
clean:
    free_path(&path);
    return error_code;
}

// ---- setting visitors: mode 0 = load from json, 1 = save to json, 2 = remember, 3 = restore ----
// cflags-free: the original used a 4-entry jump table for the mode switch
struct Visit { cJSON *json; int mode; unsigned char failed; };
FN(cJSON *, cjson_get_object_item, (const cJSON *object, const char *name));      // cJSON_GetObjectItemCaseSensitive
FN(int, cjson_is_bool, (const cJSON *object));                            // cJSON_IsBool
FN(int, cjson_is_true, (const cJSON *object));                            // cJSON_IsTrue
FN(int, cjson_is_number, (const cJSON *object));                            // cJSON_IsNumber
FN(double, cjson_get_number_value, (const cJSON *object));                         // cJSON_GetNumberValue
FN(int, cjson_is_object, (const cJSON *object));                            // cJSON_IsObject
FN(cJSON *, cjson_add_item_to_object, (cJSON *object, const char *name, int b));     // cJSON_AddBoolToObject
FN(cJSON *, cjson_add_number_to_object, (cJSON *object, const char *name, double v));  // cJSON_AddNumberToObject
FN(cJSON *, cjson_add_bool_to_object, (cJSON *object, const char *name));            // cJSON_AddObjectToObject
extern "C" const char str_min[], str_max[];   // "min", "max"
struct BoolSet { char padding_00[0x18]; unsigned char value, saved; };
extern "C" void visit_bool_setting(Visit *visitor, const char *name, BoolSet *s) {
    if (visitor->failed) return;
    switch (visitor->mode) {
    case 0: {
        unsigned char b = s->saved;
        cJSON *json = cjson_get_object_item(visitor->json, name);
        if (cjson_is_bool(json)) b = cjson_is_true(json) != 0;
        s->value = b;
        break;
    }
    case 2: s->saved = s->value; break;
    case 3: s->value = s->saved; break;
    case 1: {
        cJSON *j = visitor->json;

        visitor->failed |= cjson_add_item_to_object(j, name, s->value) == 0;
        break;
    }
    }
}
struct FloatSet { char padding_00[0x28]; float min, max, value, saved; };
extern "C" void visit_float_setting(Visit *visitor, const char *name, FloatSet *s) {
    if (visitor->failed) return;
    int mode = visitor->mode;
    if (mode == 0) {
        cJSON *json = cjson_get_object_item(visitor->json, name);
        float r = s->saved, lo = s->min, hi = s->max;
        if (cjson_is_number(json)) {
            float d = (float)cjson_get_number_value(json);
            if (d == d) {
                r = lo;
                if (!(lo > d)) {
                    r = d;
                    if (d > hi) r = hi;
                }
            }
        }
        s->value = r;
    } else {

    if (mode == 1) {
        cJSON *j = visitor->json;

        visitor->failed |= cjson_add_number_to_object(j, name, (double)s->value) == 0;
    } else if (mode == 2) {
        s->saved = s->value;
    } else if (mode == 3) {
        s->value = s->saved;
    }
    }
}
struct Range { float min, max; };
struct RangeSet { char padding_00[0x38]; float lo, hi; Range cur, saved; };
static inline __attribute__((always_inline)) float clampf(float x, float lo, float hi) {
    float r = lo;
    if (!(lo > x)) {
        r = x;
        if (x > hi) r = hi;
    }
    return r;
}
extern "C" void visit_range_setting(Visit *visitor, const char *name, RangeSet *s) {
    if (visitor->failed) return;
    int mode = visitor->mode;
    if (mode == 0) {
        cJSON *json = cjson_get_object_item(visitor->json, name);
        Range r = s->saved;
        float lo = s->lo, hi = s->hi;
        if (cjson_is_object(json)) {
            float mn = (float)cjson_get_number_value(cjson_get_object_item(json, str_min));
            if (mn == mn) {
                float mx = (float)cjson_get_number_value(cjson_get_object_item(json, str_max));
                if (mx == mx) {
                    float a = mx < mn ? mx : mn;
                    r.min = clampf(a, lo, hi);
                    r.max = clampf(mx, lo, hi);
                }
            }
        }
        s->cur = r;
    } else {

    if (mode == 1) {
        cJSON *o = cjson_add_bool_to_object(visitor->json, name);
        if (!o) { visitor->failed = 1; return; }
        visitor->failed |= cjson_add_number_to_object(o, str_min, (double)s->cur.min) == 0;
        visitor->failed |= cjson_add_number_to_object(o, str_max, (double)s->cur.max) == 0;
    } else if (mode == 2) {
        s->saved = s->cur;
    } else if (mode == 3) {
        s->cur = s->saved;
    }
    }
}

// ---- current profile name / dirty flag, profile file helpers ----
extern "C" char *settings_buffer;
extern "C" char *get_saved_string() { return settings_buffer; }
extern "C" unsigned char get_profile_dirty() { return settings_dirty_flag; }
extern "C" __attribute__((noinline)) void set_profile_dirty(unsigned char v) { settings_dirty_flag = v; }
FN(int, delete_file_path, (char **path));
FN(int, open_path_explorer, (char **path));
FN(int, find_files, (char **path, void *object, void *other_object));
extern "C" int delete_profile_file(const char *name) {   // delete profile file
    char *path;
    int error_code = settings_path(&path);
    if (error_code) return error_code;
    if ((error_code = settings_path_append(&path, str_slinky_in_profiles)) == 0 && (error_code = settings_path_append(&path, str_profiles)) == 0 &&
        (error_code = settings_path_append(&path, name)) == 0)
        error_code = delete_file_path(&path);
    free_path(&path);
    return error_code;
}
extern "C" int open_profiles_folder() {   // open the profiles folder
    char *path;
    int error_code = settings_path(&path);
    if (error_code) return error_code;
    if ((error_code = settings_path_append(&path, str_slinky_in_profiles)) == 0 && (error_code = settings_path_append(&path, str_profiles)) == 0)
        error_code = open_path_explorer(&path);
    free_path(&path);
    return error_code;
}
extern "C" int list_profiles(void *ctx, void *callback) {   // list profiles
    char *path;
    int error_code = settings_path(&path);
    if (error_code) return error_code;
    if ((error_code = settings_path_append(&path, str_slinky_in_profiles)) == 0 && (error_code = settings_path_ensure(&path)) == 0 &&
        (error_code = settings_path_append(&path, str_profiles)) == 0 && (error_code = settings_path_ensure(&path)) == 0)
        error_code = find_files(&path, ctx, callback);
    free_path(&path);
    return error_code;
}

FN(int, cjson_is_string, (const cJSON *object));                            // cJSON_IsString
FN(cJSON *, cjson_add_string_to_object, (cJSON *object, const char *name, const char *s));
extern "C" int libc_strcmp(const char *left, const char *right);      // strcmp
struct JStr { char padding_00[0x20]; const char *valuestring; };
struct ChoiceSet { char padding_00[0x20]; unsigned count; char padding_24[4]; const char **names; unsigned value, saved; };
extern "C" void visit_choice_setting(Visit *visitor, const char *name, ChoiceSet *s) {
    if (visitor->failed) return;
    int mode = visitor->mode;
    if (mode == 0) {
        unsigned r = s->saved;
        cJSON *json = cjson_get_object_item(visitor->json, name);
        if (cjson_is_string(json)) {
            unsigned long long n = s->count;
            for (unsigned long long i = 0; i < n; i++)
                if (libc_strcmp(s->names[i], ((JStr *)json)->valuestring) == 0) { r = (unsigned)i; break; }
        }
        s->value = r;
    } else {

        if (mode == 1) {
            if (!s->count) {
                visitor->failed = 1;
                return;
            }
            unsigned v = s->value < s->count ? s->value : 0;
            visitor->failed |= cjson_add_string_to_object(visitor->json, name, s->names[v]) == 0;
        } else if (mode == 2) s->saved = s->value;
        else if (mode == 3) s->value = s->saved;
    }
}
struct Color { float r, g, b; };
extern "C" const float threshold_1_in_profiles;   // 1.0
extern "C" const char str_r_in_profiles[], str_g_in_profiles[], str_b_in_profiles[];
static inline __attribute__((always_inline)) bool unit(float x) {

    return x >= 0.0f && !(x > threshold_1_in_profiles);
}
extern "C" Color parse_color(const cJSON *json, const Color *def) {
    if (cjson_is_object(json)) {
        float r = (float)cjson_get_number_value(cjson_get_object_item(json, str_r_in_profiles));
        if (unit(r)) {
            float g = (float)cjson_get_number_value(cjson_get_object_item(json, str_g_in_profiles));
            if (unit(g)) {
                float b = (float)cjson_get_number_value(cjson_get_object_item(json, str_b_in_profiles));
                if (unit(b)) return Color{r, g, b};
            }
        }
    }
    return *def;
}
struct ColorSet { char padding_00[8]; Color cur, saved; };
extern "C" void visit_color_setting(Visit *visitor, const char *name, ColorSet *s) {
    if (visitor->failed) return;
    int mode = visitor->mode;
    if (mode == 0) {
        cJSON *json = cjson_get_object_item(visitor->json, name);
        Color def = s->saved;
        s->cur = parse_color(json, &def);
    } else {

        if (mode == 1) {
            cJSON *o = cjson_add_bool_to_object(visitor->json, name);
            if (!o) { visitor->failed = 1; return; }
            visitor->failed |= cjson_add_number_to_object(o, str_r_in_profiles, (double)s->cur.r) == 0;
            visitor->failed |= cjson_add_number_to_object(o, str_g_in_profiles, (double)s->cur.g) == 0;
            visitor->failed |= cjson_add_number_to_object(o, str_b_in_profiles, (double)s->cur.b) == 0;
        } else if (mode == 2) {
            s->saved = s->cur;
        } else if (mode == 3) {
            s->cur = s->saved;
        }
    }
}

// ---- per-module json: "__on", "__bind", "__hold" and the module's own settings visitor ----
struct MCfg { const char *name; char padding_08[0xc]; unsigned char enabled; char padding_15[3]; unsigned flags; unsigned bind;
              unsigned char hold; char padding_21[0x1df]; void (*visit)(Visit *); };
FN(void, queue_module_toggle, (Mods *module_manager, MCfg *module, unsigned char enabled));
FN(void, apply_module_state, (Mods *mods, MCfg *m, int key));
extern "C" const char mc_obf_name_on[], str_bind_in_profiles[], str_hold[];
extern "C" const double offset_0_5_in_profiles;   // 0.5
struct JNum { char padding_00[0x30]; double vd; };
extern "C" unsigned char load_module_settings(cJSON *root, Mods *mods, Mod *mm) {
    MCfg *m = (MCfg *)mm;
    cJSON *json = cjson_get_object_item(root, m->name);
    unsigned f = m->flags;
    if (!(f & 2)) {
        queue_module_toggle(mods, m, cjson_is_true(cjson_get_object_item(json, mc_obf_name_on)) != 0);
        f = m->flags;
    }
    if (!(f & 1)) {
        cJSON *item = cjson_get_object_item(json, str_bind_in_profiles);
        int key;
        if (cjson_is_number(item)) {
            int k = (int)(((JNum *)item)->vd + offset_0_5_in_profiles);
            key = 0;
            if ((unsigned)(k - 0x68) >= (unsigned)-0x67) key = k;
        } else {
            key = 0;
        }
        apply_module_state(mods, m, key);
        m->hold = (m->flags & 8) ? 0 : cjson_is_true(cjson_get_object_item(json, str_hold)) != 0;
    }
    if (!m->visit) return true;
    Visit vv = {json, 0, 0};
    m->visit(&vv);
    return !vv.failed;
}
extern "C" unsigned char save_module_settings(cJSON *root, Mod *mm) {
    MCfg *m = (MCfg *)mm;
    cJSON *json = cjson_add_bool_to_object(root, m->name);
    unsigned f = m->flags;
    if (!(f & 2) && m->enabled == 1) {
        if (!cjson_add_item_to_object(json, mc_obf_name_on, 1)) return false;
        f = m->flags;
    }
    if (!(f & 1)) {
        if (m->bind) {
            if (!cjson_add_number_to_object(json, str_bind_in_profiles, (double)m->bind)) return false;
            f = m->flags;
        }
        if (!(f & 8) && m->hold == 1 && !cjson_add_item_to_object(json, str_hold, 1)) return false;
    }
    if (!m->visit) return true;
    Visit vv = {json, 1, 0};
    m->visit(&vv);
    return !vv.failed;
}

// ---- ImGui widget wrappers: mark the profile dirty when a widget changed its setting ----
FN(unsigned char, draw_toggle_row, (void *gui, void *window, const char *label, void *setting, void *value));
FN(unsigned char, draw_slider_row, (void *gui, void *window, void *label, void *format, void *setting, float min, float max, float step, float scale,
                                    void *value));
FN(unsigned char, draw_range_slider_row, (void *gui, void *window, void *label, void *format, void *range_format, void *setting, float min, float max,
                                          float step, float scale, void *low, void *high));
// bool setting: label, then the switch value
struct ToggleSetting { char padding_00[0x10]; void *label; unsigned char value; };
extern "C" void widget_bool(void *window, void *gui, ToggleSetting *setting) {
    if (draw_toggle_row(gui, window, (const char *)setting->label, setting, &setting->value)) set_profile_dirty(1);
}
// float setting: label, display format, step, display scale, bounds, current value
struct SliderSetting { char padding_00[0x10]; void *label, *format; float step, scale, min, max, value; };
extern "C" void widget_slider(void *window, void *gui, SliderSetting *setting) {
    if (draw_slider_row(gui, window, setting->label, setting->format, setting, setting->min, setting->max, setting->step, setting->scale, &setting->value)) set_profile_dirty(1);
}
// range setting: label, display formats, step, display scale, bounds, current low / high
struct RangeSetting { char padding_00[0x18]; void *label, *format, *range_format; float step, scale, min, max, low, high; };
extern "C" void widget_range(void *window, void *gui, RangeSetting *setting) {
    if (draw_range_slider_row(gui, window, setting->label, setting->format, setting->range_format, setting, setting->min, setting->max, setting->step, setting->scale, &setting->low, &setting->high)) set_profile_dirty(1);
}

// ---- list setting (names, optionally with a colour each) ----
FN(cJSON *, cjson_add_array_to_object, (cJSON *object, const char *name));           // cJSON_AddArrayToObject
FN(cJSON *, cjson_create_string, (const char *text));                        // cJSON_CreateString
FN(int, cjson_add_item_to_array, (cJSON *arr, cJSON *item));                  // cJSON_AddItemToArray
FN(int, cjson_is_array, (const cJSON *object));                           // cJSON_IsArray
extern "C" const char str_name[], str_color[];      // "name", "color"
extern "C" const double default_colour_rg_one_as_double;                       // {1.0f, 1.0f}
struct JNode2 { JNode2 *next, *prev, *child; int type; const char *vs; };
struct ListSet {
    char padding_00[0x18]; int kind; int padding_1c; void *ctx;
    unsigned long long (*count)(void *ctx);
    const char *(*name)(void *ctx, unsigned long long i);
    const float *(*color)(void *ctx, unsigned long long i);
    void (*add)(void *ctx, const char *name, Color *c);
    void (*remove)(void *ctx, unsigned long long i);
    void (*clear)(void *ctx);
};
extern "C" void visit_list_setting(Visit *visitor, const char *name, ListSet *s) {
    if (visitor->failed) return;
    int mode = visitor->mode;
    if (mode == 0) {
        s->clear(s->ctx);
        JNode2 *arr = (JNode2 *)cjson_get_object_item(visitor->json, name);
        unsigned char notarr = !cjson_is_array((cJSON *)arr);
        if ((arr == 0) | notarr) return;
        for (JNode2 *e = arr->child; e; e = e->next) {
            if (s->kind == 1) {
                if (!cjson_is_object((cJSON *)e)) continue;
                JNode2 *item = (JNode2 *)cjson_get_object_item((cJSON *)e, str_name);
                if (!cjson_is_string((cJSON *)item)) continue;
                cJSON *col = cjson_get_object_item((cJSON *)e, str_color);
                void *ctx = s->ctx;
                auto add = s->add;
                const char *str = item->vs;
                Color def;
                *(double *)&def = default_colour_rg_one_as_double;
                def.b = 1.0f;
                Color c = parse_color(col, &def);
                add(ctx, str, &c);
            } else if (s->kind == 0) {
                if (!cjson_is_string((cJSON *)e)) continue;
                Color c = {0, 0, 0};
                s->add(s->ctx, e->vs, &c);
            }
        }
    } else if (mode == 1) {
        cJSON *arr = cjson_add_array_to_object(visitor->json, name);
        if (!arr) { visitor->failed = 1; return; }
        if (!s->count(s->ctx)) return;
        for (unsigned long long i = 0; i < s->count(s->ctx); i++) {
            unsigned char bad;
            if (s->kind == 1) {
                bad = 1;
                cJSON *json = cjson_create_object();
                if (!json) goto fail;
                if (!cjson_add_string_to_object(json, str_name, s->name(s->ctx, i))) goto fail;
                {
                    cJSON *c = cjson_add_bool_to_object(json, str_color);
                    if (!c) goto fail;
                    const float *col = s->color(s->ctx, i);
                    if (!cjson_add_number_to_object(c, str_r_in_profiles, (double)col[0]) || !cjson_add_number_to_object(c, str_g_in_profiles, (double)col[1]) ||
                        !cjson_add_number_to_object(c, str_b_in_profiles, (double)col[2]))
                        goto fail;
                    bad = cjson_add_item_to_array(arr, json) == 0;
                }
            } else if (s->kind == 0) {
                cJSON *str = cjson_create_string(s->name(s->ctx, i));
                bad = str ? cjson_add_item_to_array(arr, str) == 0 : 1;
            } else {
                continue;
            }
        fail:
            visitor->failed |= bad;
        }
    } else if (mode == 3) {
        s->clear(s->ctx);
    }
}

FN(void, libc_free, (void *ptr));   // free
extern "C" void clear_saved_string() {
    if (settings_buffer) libc_free(settings_buffer);
    settings_buffer = 0;
}
