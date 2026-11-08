// 0xe000: comparators, static tables
#include "slinky/engine.h"
extern "C" int libc_strcmp(const char *left, const char *right);
extern "C" int compare_cstr_ptr(const char **left, const char **right) {
    const char *x = *left;

    return libc_strcmp(x, *right);
}
extern "C" int compare_int_field(const int *left, const char *right) { return *left - *(const int *)(right + 8); }
extern "C" int compare_int(const int *left, const int *right) { return *left - *right; }
extern "C" int compare_cstr_desc(const char **left, const char **right) {
    const char *x = *left;

    return libc_strcmp(x, *right);
}
extern "C" char default_name_list_data[], default_key_list_data[];
extern "C" void default_name_list(void **p, unsigned long long *n) { *p = default_name_list_data; *n = 0x181; }
extern "C" void default_key_list(void **p, unsigned long long *n) { *p = default_key_list_data; *n = 0xc6; }

extern "C" int compare_long_field(const char **left, const char **right) {
    long long y = *(const long long *)((const char *)right + 0x30);
    if (*(const long long *)((const char *)left + 0x30) < y) return -1;
    if (*(const long long *)((const char *)left + 0x30) > y) return 1;
    const char *x = *left;

    return libc_strcmp(x, *right);
}

FN(void *, libc_bsearch, (const void *key, const void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
struct MEnt { int id; int padding_04; unsigned long long n; void *capacity; void **list; };
struct IdMap { char padding_00[0x10]; unsigned long long count; char padding_18[8]; MEnt *data; unsigned char multi; };
// lookup id -> value (multi-maps: test membership of val)
extern "C" void *key_map_get(IdMap *m, int id, void *val) {
    if (!id) return 0;
    MEnt k = {};
    k.id = id;
    if (!m->data) return 0;
    MEnt *map_entry = (MEnt *)libc_bsearch(&k, m->data, m->count, 0x20, (int (*)(const void *, const void *))compare_int);
    if (!map_entry) return 0;
    unsigned long long n = map_entry->n;
    if (m->multi == 1) {
        for (unsigned long long i = 0; i < n; i++) {
            void *x = map_entry->list[i];
            if (x == val) return x;
        }
        return 0;
    }
    if (!n) return 0;
    return map_entry->list[0];
}

struct CbMap { unsigned char (*fn)(void *item, int id, int arg); char padding_08[8]; unsigned long long count; char padding_18[8]; MEnt *data; };
// call fn on every item registered under id; true if any returned true
extern "C" bool key_map_dispatch(CbMap *m, int id, int arg) {
    MEnt k = {};
    k.id = id;
    unsigned char r = 0;
    if (m->data) {
        MEnt *map_entry = (MEnt *)libc_bsearch(&k, m->data, m->count, 0x20, (int (*)(const void *, const void *))compare_int);
        if (map_entry && map_entry->n) {
            for (unsigned long long i = 0; i < map_entry->n; i++) r |= m->fn(map_entry->list[i], id, arg);
        }
    }
    return r & 1;
}

// ---- id -> value(s) map (sorted 32-byte entries; "multi" maps keep a list per id) ----
FN(unsigned char, vector_grow, (void *data, unsigned long long elem, unsigned long long n, void *cap));
FN(void, libc_free, (void *ptr));
FN(void *, libc_memmove, (void *dest, const void *src, unsigned long long n));
struct IdMap2 { void *fn; void (*release)(void *v); unsigned long long count, cap; MEnt *data; unsigned char multi; };
extern "C" bool key_map_add(IdMap2 *m, int id, void *val) {
    if (!id) return true;
    MEnt k = {};
    k.id = id;
    MEnt *map_entry = m->data ? (MEnt *)libc_bsearch(&k, m->data, m->count, 0x20, (int (*)(const void *, const void *))compare_int) : 0;
    if (!map_entry) {
        unsigned long long idx = 0;
        if (m->count) {
            for (idx = 0; idx < m->count; idx++)
                if (id - m->data[idx].id <= 0) break;
        }
        if (!vector_grow(&m->data, 0x20, m->count, &m->cap)) return false;
        MEnt *base = m->data;
        libc_memmove(base + idx + 1, base + idx, (m->count - idx) * 0x20);
        base[idx] = MEnt{};
        base[idx].id = id;
        m->count++;
        if (!m->data) return false;
        map_entry = m->data + idx;
    }
    unsigned long long n = map_entry->n;
    if (m->multi == 1) {
        for (unsigned long long i = 0; i < n; i++)
            if (map_entry->list[i] == val) return true;
    } else if (n) {
        if (m->release) m->release(map_entry->list[0]);
        map_entry->n = 0;
        map_entry->capacity = 0;
        libc_free(map_entry->list);
        map_entry->list = 0;
        n = map_entry->n;
    }
    if (!vector_grow(&map_entry->list, 8, n, &map_entry->capacity)) return false;
    map_entry->list[map_entry->n] = val;
    map_entry->n++;
    return true;
}
extern "C" void key_map_remove(IdMap2 *m, int id, void *val) {
    if (!id) return;
    MEnt k = {};
    k.id = id;
    if (!m->data) return;
    MEnt *map_entry = (MEnt *)libc_bsearch(&k, m->data, m->count, 0x20, (int (*)(const void *, const void *))compare_int);
    if (!map_entry) return;
    unsigned long long n0 = map_entry->n;
    if (!n0) return;
    if (m->multi == 1) {
        void **list = map_entry->list;
        unsigned long long i;
        for (i = 0; i < n0; i++)
            if (list[i] == val) break;
        if (i == n0) return;
        unsigned long long n = n0;
        if (m->release) {
            m->release(list[i]);
            n = map_entry->n;
            list = map_entry->list;
        }
        map_entry->n = --n;
        libc_memmove(&list[i], &list[i + 1], (n - i) * 8);
        if (n0 != 1) return;
    } else {
        if (m->release) m->release(map_entry->list[0]);
    }
    map_entry->n = 0;
    map_entry->capacity = 0;
    libc_free(map_entry->list);
    map_entry->list = 0;
    unsigned long long idx = (unsigned long long)(map_entry - m->data);
    unsigned long long c = m->count;
    m->count = c - 1;
    libc_memmove(map_entry, map_entry + 1, (c - idx - 1) * 0x20);
}

// ---- profile list (names + "modified" label + bind), refreshed from the profiles folder ----
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));   // calloc
FN(void *, libc_memcpy, (void *dest, const void *src, unsigned long long n));
FN(unsigned long long, libc_strlen, (const char *str));
FN(int, libc_snprintf, (char *buf, unsigned long long n, const char *fmt, ...));   // snprintf
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, list_profiles, (void *ctx, int (*callback)(void *fi, void *ctx)));
extern "C" const char str_unknown_modification_date[], str_modified_1_day_ago[], str_modified_today[], str_modified_lld_days_ago[];
struct ProfEnt { char *name; char *end; char desc[0x20]; long long age; int bind; int padding_3c; };
struct ProfList { unsigned long long count, cap; ProfEnt *data; };
struct OldEnt { char *name; int bind; int padding_0c; };
struct OldList { unsigned long long count, cap; OldEnt *data; };
struct FileInfo { const char *name; long long age; };
struct ScanCtx { ProfList *list; OldList *old; };
extern "C" int scan_next_file(FileInfo *fi, ScanCtx *ctx) {
    const char *name = fi->name;
    unsigned long long n = libc_strlen(name);
    char *s = (char *)libc_calloc(n + 1, 1);
    if (!s) return 0x3ed;
    libc_memcpy(s, name, n);
    ProfEnt e = {};
    e.name = s;
    e.end = s + (n < 0xf ? n : 0xf);
    e.age = fi->age;
    OldEnt key = {s, 0, 0};
    OldList *old = ctx->old;
    if (old->data) {
        OldEnt *f = (OldEnt *)libc_bsearch(&key, old->data, old->count, 0x10, (int (*)(const void *, const void *))compare_cstr_desc);
        if (f) e.bind = f->bind;
    }
    long long age = fi->age;
    if (age == -1) {
        __builtin_memcpy(e.desc, str_unknown_modification_date, 0x18);
    } else {
        long long days = age / 86400;
        if (days == 1) __builtin_memcpy(e.desc, str_modified_1_day_ago, 0x18);
        else if (days == 0) { __builtin_memcpy(e.desc, str_modified_today, 0x10); *(long long *)(e.desc + 0x10) = 0; }
        else libc_snprintf(e.desc, 0x19, str_modified_lld_days_ago, days);
    }
    ProfList *l = ctx->list;
    if (!vector_grow(&l->data, 0x40, l->count, &l->cap)) {
        libc_free(s);
        return 0x3ed;
    }
    l->data[l->count] = e;
    l->count++;
    return 0;
}
extern "C" int load_profiles(ProfList *l) {
    OldList old = {0, 0, 0};
    int error_code;
    if (l->count) {
        unsigned long long left = l->count;
        for (ProfEnt *e = l->data; ; ) {
            if (e->bind) {
                const char *nm = e->name;
                unsigned long long n = libc_strlen(nm);
                char *s = (char *)libc_calloc(n + 1, 1);
                if (!s) { error_code = 0x3ed; goto cleanup; }
                libc_memcpy(s, nm, n);
                int bind = e->bind;
                if (vector_grow(&old.data, 0x10, old.count, &old.cap)) {
                    old.data[old.count].name = s;
                    old.data[old.count].bind = bind;
                    old.data[old.count].padding_0c = 0;
                    old.count++;
                }
            }
            if (!--left) break;
            e = l->data + (l->count - left);
        }
        if (old.data) libc_qsort(old.data, old.count, 0x10, (int (*)(const void *, const void *))compare_cstr_desc);
        for (unsigned long long i = 0; i < l->count; i++) libc_free(l->data[i].name);
        l->count = 0;
        l->cap = 0;
        libc_free(l->data);
        l->data = 0;
    }
    {
        ScanCtx ctx = {l, &old};
        error_code = list_profiles(&ctx, (int (*)(void *, void *))scan_next_file);
        if (l->count && l->data) libc_qsort(l->data, l->count, 0x40, (int (*)(const void *, const void *))compare_long_field);
    }
cleanup:
    for (unsigned long long i = 0; i < old.count; i++) libc_free(old.data[i].name);
    old.count = 0;
    old.cap = 0;
    libc_free(old.data);
    return error_code;
}

// ---- generic dynamic arrays (as 0xdd0..0xdf90): {u32 cap, u32 count} header, element size passed in ----
struct AHdr { unsigned cap, count; };
typedef int (*ACmp)(void *ctx, const void *key, const void *elem);
FN(void *, libc_realloc, (void *ptr, unsigned long long n));   // realloc
extern "C" __attribute__((noreturn)) void libc_abort();   // abort
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));   // memcpy
// sorted find-or-insert: the existing element equal to key, or key inserted in order
extern "C" void *vector_find_insert(void **data, unsigned elem, AHdr *header, const void *key, ACmp cmp) {
    unsigned hi = header->count, idx = 0, cnt = 0;
    if (hi) {
        char *base = (char *)*data;
        unsigned lo = 0;
        for (;;) {
            unsigned mid = lo + ((hi - lo) >> 1);
            int result = cmp(0, key, base + mid * elem);
            if (result < 0) {
                hi = mid;
                if (lo >= hi) break;
            } else if (result == 0) {
                return (char *)*data + mid * elem;
            } else {
                lo = mid + 1;
                if (lo >= hi) break;
            }
        }
        cnt = header->count;
        idx = hi;
    }
    char *base = (char *)*data;
    if (cnt >= header->cap) {
        unsigned cap = header->cap ? header->cap * 2 : 8;
        header->cap = cap;
        void *p = libc_realloc(base, cap * elem);
        if (!p) libc_abort();
        *data = p;
        base = (char *)p;
        cnt = header->count;
    }
    char *at = base + idx * elem;
    libc_memmove(base + (idx + 1) * elem, at, (cnt - idx) * elem);
    libc_memcpy_alt(at, key, elem);
    header->count++;
    return at;
}
// erase index i
extern "C" void *vector_erase(void *base, unsigned elem, AHdr *header, unsigned i) {
    unsigned n = --header->count;
    return libc_memmove((char *)base + i * elem, (char *)base + (i + 1) * elem, (n - i) * elem);
}
// erase the element at p
extern "C" void *vector_erase_value(void *base, unsigned elem, AHdr *header, void *p) {
    unsigned i = (unsigned)((unsigned long long)((char *)p - (char *)base) / elem);
    unsigned n = --header->count;
    return libc_memmove((char *)base + i * elem, (char *)base + (i + 1) * elem, (n - i) * elem);
}
// make room for element n (64-bit sizes): starts at 8, doubles
extern "C" unsigned char vector_grow(void *datav, unsigned long long elem, unsigned long long n, void *capv) {
    void **data = (void **)datav;
    unsigned long long *cap = (unsigned long long *)capv;
    unsigned long long c = *cap;
    if (n < c) return true;
    void *p;
    if (*data) {
        c += c;
        p = libc_realloc(*data, elem * c);
    } else {
        c = 8;
        p = libc_calloc(8, elem);
    }
    if (!p) return false;
    *data = p;
    *cap = c;
    return true;
}

// ---- block name <-> id tables (sorted by name / by id, 0x181 entries) ----
struct NameId { const char *name; int id; int padding_0c; };
extern "C" NameId name_id_table_sorted_by_name[], name_id_table_sorted_by_id[];
extern "C" int key_name_to_code(const char *name) {
    const char *k = name;
    NameId *e = (NameId *)libc_bsearch(&k, name_id_table_sorted_by_name, 0x181, 0x10, (int (*)(const void *, const void *))compare_cstr_ptr);
    int result;
    if (e) result = e->id;
    else result = -1;
    SHARED_EPILOG();
    return result;
}
extern "C" const char *key_code_to_index(int id) {
    int k = id;
    NameId *e = (NameId *)libc_bsearch(&k, name_id_table_sorted_by_id, 0x181, 0x10, (int (*)(const void *, const void *))compare_int_field);
    const char *r;
    if (e) r = e->name;
    else r = 0;
    SHARED_EPILOG();
    return r;
}
// call fn(item, id, 0) for every registered item
extern "C" void invoke_callback(CbMap *m) {
    for (unsigned long long i = 0; i < m->count; i++) {
        MEnt *map_entry = &m->data[i];
        if (!map_entry->n) continue;
        unsigned long long j = 0;
        do {
            void *it = map_entry->list[j];

            m->fn(it, map_entry->id, 0);
        } while (++j < map_entry->n);
    }
}

// ---- profile list accessors ----
struct Profs { unsigned long long count, cap; ProfEnt *data; void *bind; };
extern "C" void *profile_set_bind(Profs *p) { return p->bind; }
extern "C" ProfEnt *profile_array(Profs *p, unsigned long long *n) {
    *n = p->count;
    return p->count ? p->data : 0;
}
extern "C" ProfEnt *find_profile(Profs *p, const char *name) {
    unsigned long long n = p->count;
    if (!n) return 0;
    ProfEnt *e = p->data;
    for (; n; n--, e++)
        if (libc_strcmp(name, e->name) == 0) return e;
    return 0;
}

// ---- gap-scan functions of the 0xe000 block ----
// grow a ring buffer when full: capacity doubles (16 first); if the contents wrap around, the wrapped part
// [0, tail) is copied behind the old end. Indices are free-running, taken modulo the (power of two) capacity.
struct Ring { unsigned long long cap, head, tail; };
extern "C" void vector_reserve(void **data, unsigned long long elem, Ring *r) {
    unsigned long long cap = r->cap;
    if (r->tail - r->head < cap) return;
    unsigned long long ncap = cap ? cap * 2 : 16;
    r->cap = ncap;
    void *p = libc_realloc(*data, ncap * elem);
    if (!p) libc_abort();
    *data = p;
    unsigned long long mask = cap - 1;
    unsigned long long h = r->head & mask, t = r->tail & mask;
    if (h < t) return;
    libc_memcpy_alt((char *)p + cap * elem, p, t * elem);
}

// small zeroed records
struct Node30 { void *a, *b; char padding_10[0x18]; unsigned char flag; char padding_29[7]; };   // 0x30
extern "C" Node30 *find_entry_index(void *jvm, void *object, bool flag) {
    Node30 *n = (Node30 *)libc_calloc(1, sizeof(Node30));
    if (n) {
        n->a = jvm;
        n->b = object;
        n->flag = flag;
    }
    return n;
}
struct Node20 { char padding_00[0x18]; void *p; };   // 0x20
extern "C" Node20 *count_matching(void *p) {
    Node20 *n = (Node20 *)libc_calloc(1, sizeof(Node20));
    n->p = p;   // (the original does not check the allocation)
    return n;
}

// destroy a callback map (see CbMap): run dtor on every registered item, free the lists, the entry array and the map
struct DtorMap { void *fn; void (*dtor)(void *item); unsigned long long count; unsigned long long cap; MEnt *data; };
extern "C" void key_map_destroy(DtorMap *m) {
    if (!m) return;
    if (m->count) {
        void (*dtor)(void *) = m->dtor;
        for (unsigned long long i = 0; i < m->count; i++) {
            MEnt *map_entry = &m->data[i];
            if (dtor && map_entry->n)
#pragma clang loop unroll(disable)
                for (unsigned long long j = 0; j < map_entry->n; j++) dtor(map_entry->list[j]);
            map_entry->n = 0;
            map_entry->capacity = 0;
            libc_free(map_entry->list);
            map_entry->list = 0;
        }
    }
    m->count = 0;
    m->cap = 0;
    libc_free(m->data);
    libc_free(m);
}

// free a profile list: every entry's name, the entry array and the list itself
extern "C" void destroy_profiles(ProfList *l) {
    if (!l) return;
#pragma clang loop unroll(disable)
    for (unsigned long long i = 0; i < l->count; i++) libc_free(l->data[i].name);
    l->count = 0;
    l->cap = 0;
    libc_free(l->data);
    libc_free(l);
}

// look up the GLFW entry points the client hooks from the already loaded "glfw" module
// (0x3e8: module not loaded, 0x3e9: a function is missing)
FN(void *, kernel32_lookups, (const char *module));
FN(void *, get_proc_address, (void *module, const char *name));
extern "C" void *pfn_glfwgetcurrentcontext, *pfn_glfwgetframebuffersize, *pfn_glfwsetframebuffersizecallback, *pfn_glfwsetmousebuttoncallback, *pfn_glfwsetkeycallback, *pfn_glfwsetcharcallback, *pfn_glfwsetcursorposcallback, *pfn_glfwsetscrollcallback;
extern "C" const char str_glfw[], str_glfwgetcurrentcontext[], str_glfwgetframebuffersize[], str_glfwsetframebuffersizecallback[], str_glfwsetmousebuttoncallback[], str_glfwsetkeycallback[],
    str_glfwsetcharcallback[], str_glfwsetcursorposcallback[], str_glfwsetscrollcallback[];
extern "C" int init_glfw_entrypoints() {
    void *glfw = kernel32_lookups(str_glfw /* "glfw" */);
    if (!glfw) return 0x3e8;
    if (!(pfn_glfwgetcurrentcontext = get_proc_address(glfw, str_glfwgetcurrentcontext /* glfwGetCurrentContext */))) return 0x3e9;
    if (!(pfn_glfwgetframebuffersize = get_proc_address(glfw, str_glfwgetframebuffersize /* glfwGetFramebufferSize */))) return 0x3e9;
    if (!(pfn_glfwsetframebuffersizecallback = get_proc_address(glfw, str_glfwsetframebuffersizecallback /* glfwSetFramebufferSizeCallback */))) return 0x3e9;
    if (!(pfn_glfwsetmousebuttoncallback = get_proc_address(glfw, str_glfwsetmousebuttoncallback /* glfwSetMouseButtonCallback */))) return 0x3e9;
    if (!(pfn_glfwsetkeycallback = get_proc_address(glfw, str_glfwsetkeycallback /* glfwSetKeyCallback */))) return 0x3e9;
    if (!(pfn_glfwsetcharcallback = get_proc_address(glfw, str_glfwsetcharcallback /* glfwSetCharCallback */))) return 0x3e9;
    if (!(pfn_glfwsetcursorposcallback = get_proc_address(glfw, str_glfwsetcursorposcallback /* glfwSetCursorPosCallback */))) return 0x3e9;
    if (!(pfn_glfwsetscrollcallback = get_proc_address(glfw, str_glfwsetscrollcallback /* glfwSetScrollCallback */))) return 0x3e9;
    return 0;
}
