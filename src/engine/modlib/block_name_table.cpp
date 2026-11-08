// 0x41000: sorted name table (24-byte entries) helpers
#include "slinky/engine.h"
struct Ent { void *name; long long q; int d; int key; };
struct Vec { unsigned long long size, cap; Ent *data; };

extern "C" unsigned long long name_table_count, name_table_capacity;
extern "C" Ent *name_table_entries;
extern "C" unsigned char safe_walk_safe_walk_active;

FN(unsigned long long, libc_strlen, (const char *str));
FN(int, key_name_to_code, (void *key));
FN(void *, key_code_to_index, (int idx));
FN(unsigned char, vector_grow, (void *vec, unsigned long long elem, unsigned long long n, void *cap));
FN(void *, libc_bsearch, (const void *key, const void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(void, libc_memmove, (void *dst, const void *src, unsigned long long n));
FN(void, libc_free, (void *ptr));
FN(void *, get_minecraft, (void *jvm));
FN(void *, get_local_player, (void *jvm, void *minecraft));
FN(void, java_set_entity_living_base_max_hurt_time, (void *jvm, void *entity_living_base, int value));

extern "C" int compare_name_key(const int *left, const Ent *right) { return *left - right->key; }
extern "C" void text_filter_name(char *buf, char character) {
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
extern "C" unsigned long long deref_u64(unsigned long long *p) { return *p; }
extern "C" unsigned long long vec24_key(Vec *vector, long long i) { return *(unsigned long long *)((char *)vector->data + i * 24); }
extern "C" void *vec24_name(Vec *vector, long long i) { return (char *)vector->data + i * 24 + 8; }
extern "C" unsigned char name_table_find(Vec *vector, void *name, long long *src) {
    int id = key_name_to_code(name);
    if (id == -1) return 0;
    if (name_table_entries && libc_bsearch(&id, name_table_entries, name_table_count, 0x18, (int (*)(const void *, const void *))compare_name_key))
        return 0;
    unsigned long long idx = 0;
    if (vector->size) {
        for (idx = 0; idx < vector->size; idx++)
            if (id <= vector->data[idx].key) break;
    }
    void *nm = key_code_to_index(id);
    long long q = src[0];
    int d = (int)src[1];
    int k = id;
    unsigned char ok = vector_grow(&name_table_entries, 0x18, name_table_count, &name_table_capacity);
    if (!ok) return ok;
    Ent *base = name_table_entries;
    libc_memmove(base + idx + 1, base + idx, (name_table_count - idx) * 0x18);
    base[idx].name = nm;
    base[idx].q = q;
    base[idx].d = d;
    base[idx].key = k;
    name_table_count++;
    return ok;
}
extern "C" void vec24_erase(Vec *vector, long long i) {
    vector->size--;
    libc_memmove(&vector->data[i], &vector->data[i + 1], (vector->size - i) * sizeof(Ent));
}
extern "C" void vec24_clear(Vec *vector) {
    vector->size = 0; vector->cap = 0;
    libc_free(vector->data);
    vector->data = 0;
}
extern "C" unsigned char get_render_flag() { return safe_walk_safe_walk_active; }
extern "C" void set_local_player_max_hurt_time(void *jvm) {
    void *player = get_local_player(jvm, get_minecraft(jvm));
    if (player) java_set_entity_living_base_max_hurt_time(jvm, player, 0x7fffffff);
}
