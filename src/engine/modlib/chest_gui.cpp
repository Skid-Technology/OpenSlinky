// 0x48000: chest-GUI check (localized title compare) and randomized click delay
#include "slinky/engine.h"
#include "slinky/jids.h"
struct Rot { float yaw, pitch; };
FN(void *, get_current_screen, (void *jvm, void *object));
FN(unsigned char, java_jni_is_instance_of_gui_chest, (void *jvm, void *candidate));
FN(void *, java_get_gui_chest_lower_chest_inventory, (void *jvm, void *gui_chest));
FN(void *, java_jni_is_instance_of_inventory_basic, (void *jvm, void *candidate));
FN(void *, java_get_inventory_basic, (void *jvm, void *inventory_basic));
FN(void *, java_call_static_stat_collector_translate_to_local, (void *jvm, void *stat_collector));
FN(Rot, rot_make_floats, (float a, float b));
FN(float, random_rotation, (Rot rotation));
FN(void, slot_index, (int id));
FN(int, libc_strncmp_alt, (const void *left, const void *right, unsigned long long n));
extern "C" unsigned char chest_stealer_require_vanilla_title_value;
extern "C" float chest_stealer_delay_after_open_value, chest_stealer_steal_delay, range_low_1_in_chest_gui, random_range_bound_1_5_in_chest_gui;
extern "C" const char str_container_chest[], str_container_chestdouble[];

// does the Java string equal the translation of `key`?
extern "C" bool java_string_equal_translation_key(Jvm *jvm, jstring s, const char *key) {
    JNIEnv *env = jvm->env;
    if (!s || !key) return false;
    jstring key_string = (*env)->NewStringUTF(env, key);
    jstring t = key_string ? (jstring)java_call_static_stat_collector_translate_to_local(jvm, key_string) : 0;
    if (key_string) (*env)->DeleteLocalRef(env, key_string);
    if (!t) return false;
    jsize len = (*env)->GetStringLength(env, s);
    if ((*env)->GetStringLength(env, t) != len) {
        (*env)->DeleteLocalRef(env, t);
        return false;
    }
    const jchar *a = (const jchar *)(*env)->GetStringChars(env, s, 0);
    const jchar *b = a ? (const jchar *)(*env)->GetStringChars(env, t, 0) : 0;
    bool eq = a && b && libc_strncmp_alt(a, b, (long long)len * 2) == 0;
    if (a) (*env)->ReleaseStringChars(env, s, a);
    if (b) (*env)->ReleaseStringChars(env, t, b);
    (*env)->DeleteLocalRef(env, t);
    return eq;
}

struct Ev { char padding_00[8]; Ctx *ctx; };
extern "C" void chest_gui_tick(Ev *event) {
    Ctx *ctx = event->ctx;
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (screen && java_jni_is_instance_of_gui_chest(ctx->jvm, screen)) {
        if (chest_stealer_require_vanilla_title_value != 1) return;
        void *jvm = ctx->jvm;
        void *inventory_basic = java_jni_is_instance_of_inventory_basic(jvm, java_get_gui_chest_lower_chest_inventory(jvm, screen));
        if (inventory_basic) {
            void *name = java_get_inventory_basic(ctx->jvm, inventory_basic);
            if (java_string_equal_translation_key((Jvm *)ctx->jvm, (jstring)name, str_container_chest)) return;
            if (java_string_equal_translation_key((Jvm *)ctx->jvm, (jstring)name, str_container_chestdouble)) return;
        }
    }
    chest_stealer_steal_delay = chest_stealer_delay_after_open_value;
    float lo = range_low_1_in_chest_gui;

    chest_stealer_steal_delay = random_rotation(rot_make_floats(lo, random_range_bound_1_5_in_chest_gui)) * chest_stealer_steal_delay;
    slot_index(0xd);
}

// item id -> {tool kind, material tier}  (kind 1/14/16/22 = axe/shovel/pickaxe/sword, 15 = shears)
struct ToolInfo { int kind, tier; };
extern "C" ToolInfo chest_gui_title_id(int id) {
    switch (id) {
    case 0x100: return ToolInfo{16, 3};
    case 0x101: return ToolInfo{14, 3};
    case 0x102: return ToolInfo{1, 3};
    case 0x10b: return ToolInfo{22, 3};
    case 0x10c: return ToolInfo{22, 1};
    case 0x10d: return ToolInfo{16, 1};
    case 0x10e: return ToolInfo{14, 1};
    case 0x10f: return ToolInfo{1, 1};
    case 0x110: return ToolInfo{22, 2};
    case 0x111: return ToolInfo{16, 2};
    case 0x112: return ToolInfo{14, 2};
    case 0x113: return ToolInfo{1, 2};
    case 0x114: return ToolInfo{22, 5};
    case 0x115: return ToolInfo{16, 5};
    case 0x116: return ToolInfo{14, 5};
    case 0x117: return ToolInfo{1, 5};
    case 0x11b: return ToolInfo{22, 4};
    case 0x11c: return ToolInfo{16, 4};
    case 0x11d: return ToolInfo{14, 4};
    case 0x11e: return ToolInfo{1, 4};
    case 0x167: return ToolInfo{15, 1};
    default: return ToolInfo{0, 0};
    }
}

FN(int, java_call_nbttag_list_tag_count, (void *jvm, void *list));
FN(void *, java_call_nbttag_list_get_compound_tag_at, (void *jvm, void *list, int i));
FN(short, java_jni_call_short_method_nbttag_compound_get_short, (void *jvm, void *tag, void *key));
extern "C" const char str_id_in_chest_gui[], str_lvl_in_chest_gui[];
// weighted sum of enchantment levels (weights indexed by enchantment id 0..62)
extern "C" unsigned short weighted_sum_enchantment_levels(Ctx *ctx, void *list, const unsigned char *weights) {
    if (!list) return 0;
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    jstring idS = (*env)->NewStringUTF(env, str_id_in_chest_gui);
    if (!idS) return 0;
    jstring lvlS = (*env)->NewStringUTF(env, str_lvl_in_chest_gui);
    if (!lvlS) {
        (*env)->DeleteLocalRef(env, idS);
        return 0;
    }
    int n = java_call_nbttag_list_tag_count(ctx->jvm, list);
    if (n <= 0) {
        (*env)->DeleteLocalRef(env, idS);
        (*env)->DeleteLocalRef(env, lvlS);
        return 0;
    }
    unsigned short sum = 0;
    for (int i = 0; i != n; i++) {
        void *tag = java_call_nbttag_list_get_compound_tag_at(ctx->jvm, list, i);
        unsigned short nbttag_compound_get_short = (unsigned short)java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, idS);
        if (nbttag_compound_get_short > 0x3e) {
            (*env)->DeleteLocalRef(env, tag);
            continue;
        }
        unsigned short w = weights[nbttag_compound_get_short];
        if (w == 0) {
            (*env)->DeleteLocalRef(env, tag);
            continue;
        }
        short lvl = java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, lvlS);
        sum += lvl < 0 ? 0 : (unsigned short)(w * lvl);
        (*env)->DeleteLocalRef(env, tag);
    }
    (*env)->DeleteLocalRef(env, idS);
    (*env)->DeleteLocalRef(env, lvlS);
    return sum;
}

extern "C" const unsigned char enchant_level_weights_c[], enchant_level_weights_b[], enchant_level_weights_a[], tier_base_levels[];
// score of an item for an inventory-sorter category (higher = better); cases decoded from the image jump tables
extern "C" int score_item_inventory_sorter_category(Ctx *ctx, int cat, int id, unsigned meta, void *nbt, unsigned char b) {
    switch (cat) {
    case 1: case 14: case 15: case 16: {
        ToolInfo ti = chest_gui_title_id(id);
        if (cat != ti.kind) return 0;
        int base = ti.tier * 100;
        return weighted_sum_enchantment_levels(ctx, nbt, enchant_level_weights_c) + base;
    }
    case 2:
        switch (id) {
        case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x11: case 0x12: case 0x14: case 0x18: case 0x23:
        case 0x2d: case 0x31: case 0x4e: case 0x57: case 0x5f: case 0x62: case 0x79: case 0x7d: case 0x85:
        case 0x9b: case 0x9f: case 0xa2:
            return 1;
        default:
            return 0;
        }
    case 3:
        if (id != 0x105) return 0;
        return weighted_sum_enchantment_levels(ctx, nbt, enchant_level_weights_b) + 1;
    case 4: return id == 0x36;
    case 5: return id == 0x170 ? 2 - b : 0;
    case 6: return id == 0x181;
    case 7: return id == 0x15a;
    case 8: return id == 0x118;
    case 9:
        if (id != 0x142) return 0;
        return (meta == 1) + 1;
    case 10: case 11: case 19: case 21: {
        if (id != 0x175) return 0;
        unsigned short base;
        switch (meta & 0xf) {
        case 1: base = 1; if (cat != 10) return 0; break;
        case 2: base = 1; if (cat != 19) return 0; break;
        case 5: base = 2; if (cat != 10) return 0; break;
        case 9: base = 1; if (cat != 21) return 0; break;
        case 11: base = 1; if (cat != 11) return 0; break;
        default: return 0;
        }
        return (unsigned short)(base + ((meta >> 6) & 1) + ((meta >> 5) & 1));
    }
    case 12: return id == 0x41;
    case 13: return id == 0x147;
    case 17: return id == 0x14c || id == 0x158;
    case 18: return id == 0x19d || id == 0x11a;
    case 20: return id == 0x103;
    case 22: {
        ToolInfo ti = chest_gui_title_id(id);
        if (ti.kind != 0x16) return 0;
        int base = tier_base_levels[ti.tier] * 40;
        return weighted_sum_enchantment_levels(ctx, nbt, enchant_level_weights_a) + base + 0xa0;
    }
    case 23: return id == 0x2e;
    case 24: return id == 0x146;
    case 25: {
        ToolInfo ti = chest_gui_title_id(id);
        unsigned short k;
        switch (ti.kind) {
        case 1: k = 3; break;
        case 14: k = 2; break;
        case 16: k = 1; break;
        case 22: k = 4; break;
        default: k = 0; break;
        }
        int base = (k + tier_base_levels[ti.tier]) * 40;
        return weighted_sum_enchantment_levels(ctx, nbt, enchant_level_weights_a) + base;
    }
    case 26: return id == 0x23;
    default: return 0;
    }
}
