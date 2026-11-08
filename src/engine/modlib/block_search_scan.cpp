// 0x40000: 24-byte entry vector accessors, comparators
#include "slinky/engine.h"
struct Ent { void *name; long long q; int d; int key; };
struct Vec { unsigned long long size, cap; Ent *data; };
extern "C" unsigned long long search_list_read_u64(unsigned long long *p) { return *p; }
extern "C" unsigned long long search_entry_key(Vec *vector, long long i) { return *(unsigned long long *)((char *)vector->data + i * 24); }
extern "C" void *search_entry_name(Vec *vector, long long i) { return (char *)vector->data + i * 24 + 8; }
extern "C" int compare_vec24_key(const int *left, const Ent *right) { return *left - right->key; }
extern "C" int compare_int_ids(const int *left, const int *right) {
    int result = left[0] - right[0];
    if (result == 0) result = left[1] - right[1];
    SHARED_EPILOG();
    return result;
}

FN(void, libc_memmove, (void *dst, const void *src, unsigned long long n));
FN(void, libc_free, (void *ptr));
struct V3 { unsigned long long size, cap; void *data; };
struct Lists { char padding_00[8]; V3 a, b, c; };
struct LVec { unsigned long long size, cap; Lists *data; };
extern "C" LVec block_esp_mesh_count_capacity;
extern "C" unsigned char block_esp_rebuild_requested;
static inline __attribute__((always_inline)) void clear_lists(Lists *l) {
    l->a.size = 0; l->a.cap = 0; libc_free(l->a.data); l->a.data = 0;
    l->b.size = 0; l->b.cap = 0; libc_free(l->b.data); l->b.data = 0;
    l->c.size = 0; l->c.cap = 0; libc_free(l->c.data); l->c.data = 0;
}
static inline __attribute__((always_inline)) void clear_all() {
    for (unsigned long long i = 0; i < block_esp_mesh_count_capacity.size; i++) clear_lists(&block_esp_mesh_count_capacity.data[i]);
    block_esp_mesh_count_capacity.size = 0; block_esp_mesh_count_capacity.cap = 0;
    libc_free(block_esp_mesh_count_capacity.data);
    block_esp_mesh_count_capacity.data = 0;
    block_esp_rebuild_requested = 1;
}
extern "C" void search_entries_erase(Vec *vector, long long i) {
    vector->size--;
    libc_memmove(&vector->data[i], &vector->data[i + 1], (vector->size - i) * sizeof(Ent));
    clear_all();
}
extern "C" void search_entries_clear(Vec *vector) {
    vector->size = 0; vector->cap = 0;
    libc_free(vector->data);
    vector->data = 0;
    clear_all();
}

typedef char v16c __attribute__((vector_size(16)));
typedef char v16cu __attribute__((vector_size(16), aligned(1)));
FN(int, key_name_to_code, (void *key));
FN(void *, key_code_to_index, (int idx));
FN(unsigned char, vector_grow, (void *vec, unsigned long long elem, unsigned long long n, void *cap));
FN(void *, libc_bsearch, (const void *key, const void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
extern "C" unsigned long long id_table_count, id_table_capacity;
extern "C" Ent *id_table_entries;
// insert a block-name entry into the sorted search list (and drop all cached meshes)
extern "C" unsigned char insert_block_name_entry_sorted(Vec *vector, void *name, long long *src) {
    int id = key_name_to_code(name);
    if (id == -1) return 0;
    if (id_table_entries && libc_bsearch(&id, id_table_entries, id_table_count, 0x18, (int (*)(const void *, const void *))compare_vec24_key))
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
    clear_all();
    unsigned char ok = vector_grow(&id_table_entries, 0x18, id_table_count, &id_table_capacity);
    if (!ok) return ok;
    Ent *base = id_table_entries;
    libc_memmove(base + idx + 1, base + idx, (id_table_count - idx) * 0x18);
    base[idx].name = nm;
    base[idx].q = q;
    base[idx].d = d;
    base[idx].key = k;
    id_table_count++;
    return ok;
}

struct Blk { unsigned char x, y, z, padding_03, padding_04, padding_05, padding_06, padding_07; Ent *ent; };
struct BMesh { char padding_00[8]; unsigned long long n, cap; Blk *b; };
FN(BMesh *, find_insert_mesh_record_chunk, (unsigned long long key));
// scan a 16x16x16 chunk section for every searched block id and append the hits to the section mesh
extern "C" void scan_16x16x16_chunk_section_searched(int cx, int sy, unsigned cz, const unsigned char *ids, BMesh **out) {
    if (!id_table_count) return;
    unsigned long long key = (unsigned)cx | ((unsigned long long)cz << 32);
    int ybase = sy << 4;
    for (unsigned long long e = 0; e < id_table_count; e++) {
        Ent *ent = &id_table_entries[e];
        char kb = (char)ent->key;
        v16c want = {kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb, kb};
        for (unsigned long long r = 0; r != 0x100; r++) {
            v16c row = *(const v16cu *)(ids + r * 16);
            unsigned mask = __builtin_ia32_pmovmskb128((v16c)(row == want));
            if (!mask) continue;
            unsigned char y = (unsigned char)(((unsigned)(r << 4) >> 8) + ybase);
            unsigned char z = (unsigned char)(r & 0xf);
            for (int x = 0; x != 0x10; x++) {
                if (!((mask >> x) & 1)) continue;
                BMesh *m = *out;
                if (!m) {
                    m = find_insert_mesh_record_chunk(key);
                    *out = m;
                    if (!m) return;
                }
                if (!vector_grow(&m->b, 0x10, m->n, &m->cap)) return;
                Blk *b = &m->b[m->n];
                b->x = (unsigned char)x; b->y = y; b->z = z;
                b->padding_03 = 0; b->padding_04 = 0; b->padding_05 = 0; b->padding_06 = 0; b->padding_07 = 0;
                b->ent = ent;
                m->n++;
            }
        }
    }
}

typedef short v8s __attribute__((vector_size(16)));
typedef short v8su __attribute__((vector_size(16), aligned(1)));
// same scan for 16-bit block data (id << 4 | meta)
extern "C" void scan_16_bit_block_data(int cx, int sy, unsigned cz, const unsigned short *ids, BMesh **out) {
    if (!id_table_count) return;
    unsigned long long key = (unsigned)cx | ((unsigned long long)cz << 32);
    int ybase = sy << 4;
    for (unsigned long long e = 0; e < id_table_count; e++) {
        Ent *ent = &id_table_entries[e];
        short kw = (short)ent->key;
        v8s want = {kw, kw, kw, kw, kw, kw, kw, kw};
#pragma clang loop unroll(disable) vectorize(disable)
        for (unsigned long long r = 0; r != 0x200; r++) {
            v8s row = *(const v8su *)(ids + r * 8);
            unsigned mask = __builtin_ia32_pmovmskb128((v16c)((row >> 4) == want));
            if (!mask) continue;
            unsigned char x = (unsigned char)((r & 1) << 3);
            unsigned char y = (unsigned char)(((unsigned)(r * 8) >> 8) + ybase);
            unsigned char z = (unsigned char)((r >> 1) & 0xf);
#pragma clang loop unroll(disable) vectorize(disable)
            for (int w = 0; w != 0x10; w += 2, x++) {
                if (!((mask >> w) & 3)) continue;
                BMesh *m = *out;
                if (!m) {
                    m = find_insert_mesh_record_chunk(key);
                    *out = m;
                    if (!m) return;
                }
                if (!vector_grow(&m->b, 0x10, m->n, &m->cap)) return;
                Blk *b = &m->b[m->n];
                b->x = x; b->y = y; b->z = z;
                b->padding_03 = 0; b->padding_04 = 0; b->padding_05 = 0; b->padding_06 = 0; b->padding_07 = 0;
                b->ent = ent;
                m->n++;
            }
        }
    }
}

extern "C" unsigned long long mesh_vector_capacity;
// find or insert the mesh record of chunk section `key` (sorted by x, then z)
extern "C" BMesh *find_insert_mesh_record_chunk(unsigned long long key) {
    Lists tmp = {};
    *(unsigned long long *)&tmp = key;
    if (block_esp_mesh_count_capacity.data) {
        void *hit = libc_bsearch(&tmp, block_esp_mesh_count_capacity.data, block_esp_mesh_count_capacity.size, 0x50,
                              (int (*)(const void *, const void *))compare_int_ids);
        if (hit) return (BMesh *)hit;
    }
    unsigned long long idx = 0;
    if (block_esp_mesh_count_capacity.size) {
        for (idx = 0; idx < block_esp_mesh_count_capacity.size; idx++) {
            const int *e = (const int *)&block_esp_mesh_count_capacity.data[idx];
            int c = (int)key - e[0];
            if (c == 0) c = (int)(key >> 32) - e[1];
            if (c <= 0) break;
        }
    }
    if (!vector_grow(&block_esp_mesh_count_capacity.data, 0x50, block_esp_mesh_count_capacity.size, &mesh_vector_capacity)) return 0;
    Lists *base = block_esp_mesh_count_capacity.data;
    libc_memmove(base + idx + 1, base + idx, (block_esp_mesh_count_capacity.size - idx) * 0x50);
    *(unsigned long long *)&base[idx] = key;
    base[idx].a = V3{}; base[idx].b = V3{}; base[idx].c = V3{};
    block_esp_mesh_count_capacity.size++;
    return (BMesh *)&block_esp_mesh_count_capacity.data[idx];
}

// ---- world events: chunk unload (0) / load (1) rebuild the chunk's meshes, block change (2) edits one ----
#include "slinky/jids.h"
struct WEv { Jvm *jvm; jobject world; unsigned type; int a, b, c, id; };
FN(jobject, java_world_get_chunk_from_chunk_coords, (Jvm *jvm, jobject chunk, int cx, int cz));
FN(jobject, java_get_chunk_storage_arrays, (Jvm *jvm, jobject chunk));
FN(jobject, java_get_id_b20, (Jvm *jvm, jobject sec));
FN(int, chunk_coordinate, (int x));
FN(int, block_coordinate, (int x));
FN(void, rebuild_block_esp_mesh, (BMesh *m));
FN(void, free_block_search_section_lists, (Lists *l));
extern "C" void (*block_esp_block_scan_callback)(int cx, int sy, int cz, void *ids, BMesh **out);
static inline __attribute__((always_inline)) void drop_rec(Lists *dst) {
    unsigned long long i = dst - block_esp_mesh_count_capacity.data;
    block_esp_mesh_count_capacity.size--;
    libc_memmove(dst, dst + 1, (block_esp_mesh_count_capacity.size - i) * 0x50);
}
extern "C" void equipment_event(WEv *ev) {
    if (ev->type >= 2) {
        if (ev->type != 2) return;
        int id = ev->id;
        Ent *ent = id_table_entries ? (Ent *)libc_bsearch(&id, id_table_entries, id_table_count, 0x18,
                                                   (int (*)(const void *, const void *))compare_vec24_key)
                               : 0;
        int cx = chunk_coordinate(ev->a), cz = chunk_coordinate(ev->c);
        unsigned long long key = (unsigned)cx | (unsigned long long)(unsigned)cz << 32;
        Lists tmp = {};
        *(unsigned long long *)&tmp = key;
        BMesh *m = block_esp_mesh_count_capacity.data ? (BMesh *)libc_bsearch(&tmp, block_esp_mesh_count_capacity.data, block_esp_mesh_count_capacity.size, 0x50,
                                                         (int (*)(const void *, const void *))compare_int_ids)
                                    : 0;
        unsigned char lx = ev->a - block_coordinate(cx), lz = ev->c - block_coordinate(cz), y = (unsigned char)ev->b;
        if (m) {
            unsigned long long n = m->n;
            for (unsigned long long i = 0; i < n; i++) {
                Blk *dst = &m->b[i];
                if (dst->x != lx || dst->y != y || dst->z != lz) continue;
                if (ent) {
                    dst->ent = ent;
                } else {
                    m->n = --n;
                    libc_memmove(dst, dst + 1, (n - i) * sizeof(Blk));
                    if (!m->n) {
                        free_block_search_section_lists((Lists *)m);
                        drop_rec((Lists *)m);
                        return;
                    }
                }
                rebuild_block_esp_mesh(m);
                return;
            }
            if (!ent) return;
        } else {
            if (!ent) return;
            unsigned long long idx = 0;
            if (block_esp_mesh_count_capacity.size) {
                for (idx = 0; idx < block_esp_mesh_count_capacity.size; idx++) {
                    const int *e = (const int *)&block_esp_mesh_count_capacity.data[idx];
                    int c = cx - e[0];
                    if (c == 0) c = cz - e[1];
                    if (c <= 0) break;
                }
            }
            if (!vector_grow(&block_esp_mesh_count_capacity.data, 0x50, block_esp_mesh_count_capacity.size, &mesh_vector_capacity)) return;
            Lists *base = block_esp_mesh_count_capacity.data;
            libc_memmove(base + idx + 1, base + idx, (block_esp_mesh_count_capacity.size - idx) * 0x50);
            *(unsigned long long *)&base[idx] = key;
            base[idx].a = V3{}; base[idx].b = V3{}; base[idx].c = V3{};
            block_esp_mesh_count_capacity.size++;
            if (!block_esp_mesh_count_capacity.data) return;
            m = (BMesh *)&block_esp_mesh_count_capacity.data[idx];
        }
        Blk nb = {lx, y, lz, 0, 0, 0, 0, 0, ent};
        if (!vector_grow(&m->b, 0x10, m->n, &m->cap)) return;
        m->b[m->n] = nb;
        m->n++;
        rebuild_block_esp_mesh(m);
        return;
    }
    Lists tmp = {};
    *(unsigned long long *)&tmp = *(unsigned long long *)&ev->a;
    if (block_esp_mesh_count_capacity.data) {
        Lists *r = (Lists *)libc_bsearch(&tmp, block_esp_mesh_count_capacity.data, block_esp_mesh_count_capacity.size, 0x50,
                                      (int (*)(const void *, const void *))compare_int_ids);
        if (r) {
            clear_lists(r);
            drop_rec(r);
        }
    }
    if (ev->type != 1) return;
    Jvm *jvm = ev->jvm;
    int cx = ev->a, cz = ev->b;
    jobject chunk = java_world_get_chunk_from_chunk_coords(jvm, ev->world, cx, cz);
    if (!chunk) return;
    BMesh *out = 0;
    JNIEnv *env = jvm->env;
    jobjectArray secs = (jobjectArray)java_get_chunk_storage_arrays(jvm, chunk);
    if (!secs) return;
    for (int i = 0; i != 16; i++) {
        jobject sec = (*env)->GetObjectArrayElement(env, secs, i);
        if (!sec) continue;
        jarray ids = (jarray)java_get_id_b20(jvm, sec);
        if (!ids) continue;
        void *p = (*env)->GetPrimitiveArrayCritical(env, ids, 0);
        if (!p) continue;
        block_esp_block_scan_callback(cx, i, cz, p, &out);
        (*env)->ReleasePrimitiveArrayCritical(env, ids, p, 0);
    }
    if (out) rebuild_block_esp_mesh(out);
    return;
}
