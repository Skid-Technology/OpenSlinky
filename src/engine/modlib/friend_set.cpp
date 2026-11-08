// 0x3b000: friend-list style 32-byte key set (sorted vector) with chat feedback
#include "slinky/engine.h"
struct Key { unsigned long long hi, lo; };
struct Ent { char b[0x20]; };
extern "C" void *friend_set_friend_table; extern "C" char friend_set_table_meta[]; extern "C" int friend_set_dirty_count;
extern "C" const char str_sect_aadded_s_as_friend[], str_sect_aremoved_s_as_friend[], empty_plural_suffix[], str_s_in_friend_set[], str_sect_aremoved_zu_friend_s[];
FN(unsigned char, friend_resolve, (void *c, Key k, Ent *e, char *name));
FN(void *, vector_find_insert, (void *t, int elem, void *hdr, void *e, void *cmp));
FN(void *, binary_search, (void *t, int elem, void *hdr, void *e, void *cmp));
FN(void, vector_erase_value, (void *t, int elem, void *hdr, void *at));
extern "C" void jni_throw_format(void *jvm, const char *fmt, ...);

extern "C" int compare_friend_key(void *, unsigned long long *right, unsigned long long *b) {
    unsigned long long x = __builtin_bswap64(right[0]);
    asm("" : "+r"(x));

    unsigned long long y = __builtin_bswap64(b[0]);
    if (x == y) {
        unsigned long long a1 = right[1], b1 = b[1];
        asm("" : "+r"(a1), "+r"(b1));

        x = __builtin_bswap64(a1);
        y = __builtin_bswap64(b1);
        asm("" : "+r"(x), "+r"(y));

        if (x == y) return 0;
    }
    return x >= y ? 1 : -1;
}
extern "C" unsigned char friend_set_insert(void **c, Key k) {
    Ent e; char name[0x20];
    unsigned char ok = friend_resolve(c, k, &e, name);
    if (!ok) return ok;
    vector_find_insert(&friend_set_friend_table, 0x20, friend_set_table_meta, &e, (void *)compare_friend_key);
    jni_throw_format(*c, str_sect_aadded_s_as_friend, name);
    return ok;
}
extern "C" unsigned char friend_set_erase(void **c, Key k) {
    Ent e; char name[0x20];
    unsigned char ok = friend_resolve(c, k, &e, name);
    if (!ok) return ok;
    void *entry = binary_search(friend_set_friend_table, 0x20, friend_set_table_meta, &e, (void *)compare_friend_key);
    if (entry) vector_erase_value(friend_set_friend_table, 0x20, friend_set_table_meta, entry);
    jni_throw_format(*c, str_sect_aremoved_s_as_friend, name);
    return ok;
}
extern "C" bool friend_set_clear(void **c) {
    unsigned long long n = (unsigned)friend_set_dirty_count;

    void *jvm = *c;
    jni_throw_format(jvm, str_sect_aremoved_zu_friend_s, n, n == 1 ? empty_plural_suffix : str_s_in_friend_set);
    friend_set_dirty_count = 0;
    return true;
}
extern "C" int friend_set_dirty() { return friend_set_dirty_count; }

#include "slinky/jids.h"
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(jstring, java_call_entity_get_command_sender_name, (void *jvm, void *entity));
FN(void *, java_get_entity_uuid, (void *jvm, void *entity));
FN(unsigned long long, java_jni_call_long_method_uuid_get_most_significant_bits, (void *jvm, void *uuid));
FN(unsigned long long, java_jni_call_long_method_uuid_get_least_significant_bits, (void *jvm, void *uuid));
extern "C" const char str_sect_cusername_is_too_long[], str_sect_cfound_multiple_names_sta[], str_sect_cplayer_not_found[];
struct Str { const char *s; unsigned long long len; };
struct Uid { unsigned long long hi, lo, z0, z1; };
struct C3b { Jvm *jvm; char padding_08[0x18]; void *world; void *self; };
static inline __attribute__((always_inline)) unsigned char fold(unsigned char ch) {
    return (unsigned char)(((unsigned char)(ch - 0x41) < 0x1a) << 5) + ch;
}
// resolve a (unique, case-insensitive) player-name prefix to its UUID; prints the reason on failure
extern "C" unsigned char friend_resolve(void *cv, Key kk, Ent *eo, char *name) {
    C3b *c = (C3b *)cv;
    Str k = __builtin_bit_cast(Str, kk);
    Uid *out = (Uid *)eo;
    unsigned long long len = k.len;
    if (len >= 0x11) { jni_throw_format(c->jvm, str_sect_cusername_is_too_long); return false; }
    const char *s = k.s;
    JNIEnv *env = c->jvm->env;
    char buf[0x18];
    libc_memcpy_alt(buf, s, len);
    buf[len] = 0;
    if (!(*env)->NewStringUTF(env, buf)) return false;
    void *list = java_get_world_player_entities(c->jvm, c->world);
    if (java_call_list_size(c->jvm, list) <= 0) { jni_throw_format(c->jvm, str_sect_cplayer_not_found); return false; }
    void *found = 0;
    for (int i = 0; i < java_call_list_size(c->jvm, list); i++) {
        void *element = java_call_list_get(c->jvm, list, i);
        JNIEnv *e2 = c->jvm->env;
        if ((*e2)->IsSameObject(e2, (jobject)element, (jobject)c->self)) continue;
        jstring nm = java_call_entity_get_command_sender_name(c->jvm, element);
        int L = (*env)->GetStringUTFLength(env, nm);
        if (len > (unsigned long long)(long long)L) continue;
        const char *chars = (*env)->GetStringUTFChars(env, nm, 0);
        unsigned long long j;
        for (j = 0; j < len; j++)
            if (fold(s[j]) != fold(chars[j])) break;
        if (j == len) {
            if (found) {
                (*env)->ReleaseStringUTFChars(env, nm, chars);
                jni_throw_format(c->jvm, str_sect_cfound_multiple_names_sta, (int)len, s);
                return false;
            }
            int n2 = L >= 0x10 ? 0x10 : L;
            libc_memcpy_alt(name, chars, (long long)n2);
            name[n2] = 0;
            found = element;
        }
        (*env)->ReleaseStringUTFChars(env, nm, chars);
    }
    if (!found) { jni_throw_format(c->jvm, str_sect_cplayer_not_found); return false; }
    void *uuid = java_get_entity_uuid(c->jvm, found);
    unsigned long long hi = java_jni_call_long_method_uuid_get_most_significant_bits(c->jvm, uuid);
    out->lo = java_jni_call_long_method_uuid_get_least_significant_bits(c->jvm, uuid);
    out->hi = hi;
    out->z0 = 0; out->z1 = 0;
    return true;
}

// hex string of friend entry i (first 24 bytes: the UUID + name hash), in a static buffer
FN(unsigned char *, element_at, (void *vec, unsigned elem, void *hdr, unsigned i));
extern "C" const char str_0123456789abcdef[];   // "0123456789abcdef"
extern "C" char friend_hex_buffer[];
extern "C" char *friend_set_count(void *, unsigned i) {
    const unsigned char *e = element_at(friend_set_friend_table, 0x20, friend_set_table_meta, i);
    for (int k = 0; k < 0x18; k++) {
        friend_hex_buffer[2 * k] = str_0123456789abcdef[e[k] >> 4];
        friend_hex_buffer[2 * k + 1] = str_0123456789abcdef[e[k] & 0xf];
    }
    friend_hex_buffer[0x30] = 0;
    return friend_hex_buffer;
}

// add every player within `radius` blocks as a friend (optionally clearing the list first)
struct V3b { double x, y, z; };
FN(V3b, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mask));
FN(double, vec3_distance_sq, (V3b a, V3b b));
extern "C" const char str_sect_aremoved_zu_added_zu_frie[], str_sect_aadded_zu_friend_s[];
struct NearArgs { unsigned long long radius; char padding_08[8]; unsigned char clear; };
typedef int v4i3 __attribute__((vector_size(16)));
typedef double v2d3 __attribute__((vector_size(16)));
extern "C" const v4i3 friend_set_0x43300000_0x45300000_0_0;   // {0x43300000, 0x45300000, 0, 0}
extern "C" const v2d3 friend_set_2_52_2_84;   // {2^52, 2^84}
// uint64 -> double with the image's constants (the original's lowering)
static inline __attribute__((always_inline)) double u64_to_d(unsigned long long x) {
    v4i3 v = {(int)x, (int)(x >> 32), 0, 0};
    v4i3 u = __builtin_shufflevector(v, friend_set_0x43300000_0x45300000_0_0, 0, 4, 1, 5);
    v2d3 d = (v2d3)u - friend_set_2_52_2_84;
    return d[1] + d[0];
}
extern "C" bool friend_set_contains(C3b *c, NearArgs *a) {
    unsigned long long radius = a->radius;
    unsigned char clear = a->clear;
    unsigned long long old = (unsigned)friend_set_dirty_count;
    if (clear == 1) friend_set_dirty_count = 0;
    V3b eye = java_get_entity_pos_x(c->jvm, c->self);
    void *list = java_get_world_player_entities(c->jvm, c->world);
    unsigned long long added = 0;
    if (java_call_list_size(c->jvm, list) > 0) {
        double r = u64_to_d(radius);
        r *= r;
        for (int i = 0; i < java_call_list_size(c->jvm, list); i++) {
            void *element = java_call_list_get(c->jvm, list, i);
            JNIEnv *env = c->jvm->env;
            if ((*env)->IsSameObject(env, (jobject)element, (jobject)c->self)) continue;
            if (entity_flags_has(c->jvm, element, 2)) continue;
            V3b position = java_get_entity_pos_x(c->jvm, element);
            if (vec3_distance_sq(eye, position) > r) continue;
            void *uuid = java_get_entity_uuid(c->jvm, element);
            Uid k;
            k.hi = java_jni_call_long_method_uuid_get_most_significant_bits(c->jvm, uuid);
            k.lo = java_jni_call_long_method_uuid_get_least_significant_bits(c->jvm, uuid);
            k.z0 = 0;
            k.z1 = 0;
            void *existing = binary_search(friend_set_friend_table, 0x20, friend_set_table_meta, &k, (void *)compare_friend_key);
            vector_find_insert(&friend_set_friend_table, 0x20, friend_set_table_meta, &k, (void *)compare_friend_key);
            if (!existing) added++;
        }
    }
    const char *s = added == 1 ? empty_plural_suffix : str_s_in_friend_set;
    if (clear) jni_throw_format(c->jvm, str_sect_aremoved_zu_added_zu_frie, old, added, s);
    else jni_throw_format(c->jvm, str_sect_aadded_zu_friend_s, added, s);
    return true;
}

// add a friend entry from its hex string (inverse of friend_set_count); a bad digit stops that part of the parse
FN(unsigned long long, libc_strlen, (const char *str));
static inline __attribute__((always_inline)) bool hexdig(unsigned char c, unsigned char *v) {
    if ((unsigned char)(c - '0') < 10) { *v = c - '0'; return true; }
    if ((unsigned char)(c - 'a') < 26) { *v = c - 'a' + 10; return true; }
    return false;
}
static inline __attribute__((always_inline)) void unhex(unsigned char *d, const char *s, int n) {
    for (int i = 0; i < n; i++) {
        unsigned char h, l;
        if (!hexdig(s[i * 2], &h) || !hexdig(s[i * 2 + 1], &l)) return;
        d[i] = h << 4 | l;
    }
}
extern "C" bool friend_name_matches(void *, const char *s) {
    Ent e = {};
    unsigned long long n = libc_strlen(s);
    if (n >= 0x30) {
        unhex((unsigned char *)e.b, s, 16);
        unhex((unsigned char *)e.b + 16, s + 32, 8);
        vector_find_insert(&friend_set_friend_table, 0x20, friend_set_table_meta, &e, (void *)compare_friend_key);
    }
    return n >= 0x30;
}
