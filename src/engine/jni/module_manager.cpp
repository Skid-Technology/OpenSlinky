// 0x28000: module manager - per-event handler lists, dispatch, small item helpers
// cflags: /Ob1 /clang:-fno-slp-vectorize -mllvm -switch-to-lookup=false
#include "slinky/engine.h"
#include "slinky/jcall.h"
extern "C" void slinky_logf(const char *fmt, ...);
typedef void (*Handler)(void *event);
struct Module;
struct Reg { Module **list; unsigned long long n; };
struct Mgr {
    Module **mods; unsigned long long count; Reg cat[5];   // +0x10: modules per category
    Reg reg[27];                        // +0x060: modules that implement each event type
    Handler *lists[27];                 // +0x210: NULL-terminated handler list per event type
    Module **en_list; unsigned long long en_count;    // +0x2e8 pending enables
    Module **dis_list; unsigned long long dis_count;  // +0x2f8 pending disables
    void (*set308)(void *mod, int v);   // +0x308
};
struct HSlot { Handler f; void *x; };
struct Module { char padding_00[0x10]; unsigned cat; unsigned char enabled; char padding_15[0x33]; HSlot h[27]; };
// dispatch: call every handler registered for the event's type (first dword of the event)
extern "C" void dispatch_event(Mgr *manager, unsigned *e) {
    unsigned type = *e;
    if (type >= 27) return;
    Handler *h = manager->lists[type];
    for (Handler f = *h; f; f = *++h) f(e);
}
extern "C" bool is_event_handled(Mgr *manager, unsigned type) {
    if (type >= 27) return true;
    return *manager->lists[type] == 0;
}
extern "C" bool has_queued_modules(Mgr *manager) { return manager->dis_count != 0 || manager->en_count != 0; }
extern "C" void apply_module_state(Mgr *manager, void *mod, int v) {
    auto set308 = manager->set308;
    asm("" : "+r"(set308));
    set308(mod, v);
}

FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inventory_player));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *item_stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
// item held in a hotbar slot (0..8) of the entity's inventory
extern "C" void *item_held_hotbar_slot(Jvm *jvm, void *ent) {
    void *inv = java_get_entity_player_inventory(jvm, ent);
    unsigned slot = java_get_inventory_player_current_item(jvm, inv);
    if (slot > 8) return 0;
    void *arr = java_get_inventory_player_main_inventory(jvm, inv);
    jobject st = (*jvm->env)->GetObjectArrayElement(jvm->env, (jobjectArray)arr, slot);
    if (!st) return 0;
    return java_call_item_stack_get_item(jvm, st);
}
// is the item a sword (ids 267, 268, 272, 276, 283)?
extern "C" bool item_sword(Jvm *jvm, void *item) {
    unsigned char r;
    if (!item) { r = 0; goto out; }
    {
    unsigned k = java_call_static_item_get_id_from_item(jvm, item) - 0x10b;
    if (k <= 0x10) { asm volatile(""); r = (unsigned char)(0x10223u >> k); }
    else r = 0;
    }
out:
    SHARED_EPILOG();
    return r & 1;
}

static inline __attribute__((always_inline)) bool is_code(unsigned short c) {
    return (unsigned short)(c - 0x61) < 6 | (unsigned short)(c - 0x30) < 10 | c == 0x72;
}
// colour code (0-9, a-f, r) in effect before the first name character of a display string, 0 if none
static inline __attribute__((always_inline)) unsigned prefix_code(Jvm *jvm, jstring s) {
    JNIEnv *env = jvm->env;
    if (!s) return 0;
    jsize len = (*env)->GetStringLength(env, s);
    if (len <= 0) return 0;
    const jchar *ch = (const jchar *)(*env)->GetStringCritical(env, s, 0);
    if (!ch) return 0;
    unsigned r = 0;
    bool prev = false;
    long long i = 0;
    for (;;) {
        bool p = prev;
        unsigned short c = ch[i];
        prev = c == 0xa7;
        if (!prev) {
            if (p) {
                if ((unsigned short)(c - 0x30) < 10) r = c;
                if ((unsigned short)(c - 0x61) < 6) r = c;
                if (c == 0x72) r = c;
            } else {
                if (c == 0x5f || (unsigned short)(c - 0x30) < 10 || (unsigned short)((c & 0xffdf) - 0x41) < 0x1a)
                    break;
            }
        }
        i++;
        if (i >= (long long)len) { r = 0; break; }
    }
    (*env)->ReleaseStringCritical(env, s, ch);
    return r;
}
// last colour code (0-9, a-f, r) of a display string, 'f' if none
static inline __attribute__((always_inline)) unsigned short last_code(Jvm *jvm, jstring s) {
    unsigned short res = 0x66;
    JNIEnv *env = jvm->env;
    jsize len = (*env)->GetStringLength(env, s);
    if (!len) return res;
    const jchar *ch = (const jchar *)(*env)->GetStringCritical(env, s, 0);
    if (!ch) return res;
    if (len > 0) {
        unsigned cand = 0;
        unsigned char valid = 0;
        for (unsigned long long k = (unsigned)len + 1; ; ) {
            unsigned short c = ch[k - 2];
            if (c != 0xa7) {
                valid = is_code(c);
                if (valid) cand = c;
            } else {
                if (valid & 1) { res = (unsigned short)cand; break; }
                valid = 0;
            }
            k--;
            if (k <= 1) break;
        }
    }
    (*env)->ReleaseStringCritical(env, s, ch);
    return res;
}
extern "C" unsigned name_prefix_code(Jvm *jvm, jstring s) { return prefix_code(jvm, s); }
FN(jstring, java_get_score_player_team, (Jvm *jvm, void *score_player_team));
extern "C" unsigned short name_last_code(Jvm *jvm, void *object) {
    jstring score_player_team = java_get_score_player_team(jvm, object);
    if (!score_player_team) return 0x66;
    return last_code(jvm, score_player_team);
}
FN(jstring, java_call_entity_get_command_sender_name, (Jvm *jvm, void *ent));
FN(void *, java_jni_is_instance_of_entity_living_base, (Jvm *jvm, void *ent));
FN(void *, java_call_entity_living_base_get_team, (Jvm *jvm, void *entity_living_base));
FN(void *, java_jni_is_instance_of_score_player_team, (Jvm *jvm, void *candidate));
// entity name colour: prefix code of the display name, else the team colour, else 'f'
extern "C" unsigned short entity_name_colour(Jvm *jvm, void *ent) {
    unsigned short r = (unsigned short)prefix_code(jvm, java_call_entity_get_command_sender_name(jvm, ent));
    if (r) return r;
    void *entity_living_base = java_jni_is_instance_of_entity_living_base(jvm, ent);
    if (!entity_living_base) return 0x66;
    void *entity_living_base_get_team = java_call_entity_living_base_get_team(jvm, entity_living_base);
    if (!entity_living_base_get_team) return 0x66;
    void *score_player_team = java_jni_is_instance_of_score_player_team(jvm, entity_living_base_get_team);
    if (!score_player_team) return 0x66;
    jstring s = java_get_score_player_team(jvm, score_player_team);
    if (!s) return 0x66;
    return last_code(jvm, s);
}

// apply pending enables/disables: rebuild the handler lists of the affected event types, fire event 5
// Same layout as the context the frame code builds (java_natives_frame.cpp): toggle handlers read the game
// objects, so these first fields have to line up.
struct HCtx { void *jvm; Mgr *module_manager; void *profile_list; void *minecraft; void *world; void *player; };
struct ToggleEvent { long long type; HCtx *ctx; Module *mod; unsigned char on; char rest[0x2f]; };
FN(void, release_owner, (void *timer));
static inline __attribute__((always_inline)) unsigned long long handler_mask(Module *module) {
    unsigned long long mask = 0;
#pragma clang loop unroll_count(3)
    for (unsigned t = 0; t < 27; t++)
        if (module->h[t].f) mask |= 1u << t;
    return mask;
}
static inline __attribute__((always_inline)) void rebuild(Mgr *manager, unsigned long long mask) {
#pragma clang loop unroll(disable)
    for (unsigned long long t = 0; t != 27; t++) {
        if (!((mask >> t) & 1)) continue;
        Handler *out = manager->lists[t];
        Reg *r = &manager->reg[t];
#pragma clang loop unroll_count(2)
        for (unsigned long long i = 0; i < r->n; i++) {
            Module *mod = r->list[i];
            if (mod->enabled == 1 && mod->h[t].f) *out++ = mod->h[t].f;
        }
        *out = 0;
    }
}
extern "C" void apply_module_toggles(HCtx *ctx) {
    Mgr *manager = ctx->module_manager;
    // The handlers can touch the game objects: Block In's enable reads the local player's rotation, and the
    // start-up queue (modules with flags bit 1) is applied on the first frame, which on a fresh launch is
    // the main menu, where Minecraft.thePlayer/theWorld are NULL. Keeping the queue until the first frame
    // that has both is what the tick path already does (tick_modules has the same check); applying it
    // against a NULL player faulted in jni_fast_GetFloatField.
    if (!ctx->world || !ctx->player) return;
    if (manager->en_count) {
        unsigned long long mask = 0;
        for (unsigned long long i = 0; i != manager->en_count; i++) {
            Module *mod = manager->en_list[i];
            mod->enabled = 1;
            mask |= handler_mask(mod);
        }
        rebuild(manager, mask);
        slinky_logf("toggles: enable n=%llu mask=%llx first=%s", manager->en_count, mask, *(const char **)manager->en_list[0]);
        for (unsigned long long i = 0; i < manager->en_count; i++) {
            ToggleEvent e = {5, ctx, manager->en_list[i], 1, {}};
            Handler *h = ctx->module_manager->lists[5];
            for (Handler f = *h; f; f = *++h) f(&e);
        }
        manager->en_count = 0;
        manager = ctx->module_manager;
    }
    if (manager->dis_count) {
        unsigned long long mask = 0;
        for (unsigned long long i = 0; i != manager->dis_count; i++) {
            Module *mod = manager->dis_list[i];
            mod->enabled = 0;
            mask |= handler_mask(mod);
        }
        for (unsigned long long i = 0; i < manager->dis_count; i++) {
            Module *mod = manager->dis_list[i];
            ToggleEvent e = {5, ctx, mod, 0, {}};
            Handler *h = ctx->module_manager->lists[5];
            for (Handler f = *h; f; f = *++h) f(&e);
            release_owner(mod);
        }
        rebuild(manager, mask);
        slinky_logf("toggles: disable n=%llu mask=%llx first=%s", manager->dis_count, mask, *(const char **)manager->dis_list[0]);
        manager->dis_count = 0;
    }
}

// manager constructor: copy the static module table, bucket modules by category and by implemented event
FN(void *, libc_memset, (void *dest, int c, unsigned long long n));          // memset
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));  // calloc
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));  // memcpy
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));   // qsort
extern "C" Module *const ptr_to_aim_assist_module_descriptor[0x43];
extern "C" unsigned module_manager_count;
FN(int, compare_module_slots, (const void *left, const void *right));
extern "C" int create_module_manager(Mgr *manager, void (*setter)(void *mod, int v)) {
    libc_memset(manager->cat, 0, 0x300);
    manager->count = 0x43;
    manager->mods = (Module **)libc_calloc(0x43, 8);
    if (!manager->mods) return 0x3ed;
    libc_memcpy_alt(manager->mods, ptr_to_aim_assist_module_descriptor, 0x218);
#pragma clang loop unroll(disable)
    for (unsigned long long i = 0; i != 0x43; i++) {
        Module *mod = ptr_to_aim_assist_module_descriptor[i];
        *(unsigned long long *)((char *)manager + 0x18 + ((unsigned long long)mod->cat << 4)) += 1;
#pragma clang loop unroll_count(2)
        for (unsigned t = 0; t < 27; t++)
            if (mod->h[t].f) manager->reg[t].n++;
    }
    for (int c = 0; c < 5; c++) {
        unsigned long long n = manager->cat[c].n;
        if (n) {
            manager->cat[c].list = (Module **)libc_calloc(n, 8);
            if (!manager->cat[c].list) return 0x3ed;
        } else {
            manager->cat[c].list = 0;   // empty category: calloc(0) is implementation-defined
        }
        manager->cat[c].n = 0;
    }
#pragma clang loop unroll(disable)
    for (int t = 0; t < 27; t++) {
        unsigned long long n = manager->reg[t].n;
        if (n) {
            manager->reg[t].list = (Module **)libc_calloc(n, 8);
            if (!manager->reg[t].list) return 0x3ed;
        } else {
            manager->reg[t].list = 0;
        }
        manager->lists[t] = (Handler *)libc_calloc(n + 1, 8);
        if (!manager->lists[t]) return 0x3ed;
        manager->reg[t].n = 0;
    }
    manager->en_list = (Module **)libc_calloc(0x43, 8);
    if (!manager->en_list) return 0x3ed;
    manager->dis_list = (Module **)libc_calloc(0x43, 8);
    if (!manager->dis_list) return 0x3ed;
#pragma clang loop unroll(disable)
    for (unsigned long long i = 0; i != 0x43; i++) {
        Module *mod = ptr_to_aim_assist_module_descriptor[i];
        Reg *cr = &manager->cat[mod->cat];
        unsigned long long cn = cr->n;
        cr->list[cn] = mod;
        cr->n = cn + 1;
#pragma clang loop unroll_count(2)
        for (unsigned t = 0; t < 27; t++)
            if (mod->h[t].f) {
                unsigned long long rn = manager->reg[t].n;
                manager->reg[t].list[rn] = mod;
                manager->reg[t].n = rn + 1;
            }
    }
#pragma clang loop unroll(disable)
    for (unsigned t = 0; t != 27; t++) {
        module_manager_count = t;
        libc_qsort(manager->reg[t].list, manager->reg[t].n, 8, compare_module_slots);
    }
    manager->set308 = setter;
    return 0;
}

// queue a module enable/disable (applied by apply_module_toggles); a disable is announced first (event 4) and can be
// cancelled by a handler setting the event's flag
struct QEvent { int type; int padding_04; Module *mod; unsigned char cancel; char rest[0x37]; };
extern "C" bool queue_module_toggle(Mgr *module_manager, Module *module, unsigned char enabled) {
    if (module->enabled == enabled) return true;
    Module ***list;
    if (enabled) {
        list = &module_manager->en_list;
    } else {
        QEvent ev = {};
        ev.type = 4;
        ev.mod = module;
        Handler *h = module_manager->lists[4];
        if (*h) {
            for (Handler f = *h; f; f = *++h) f((unsigned *)&ev);
            if (ev.cancel & 1) return false;
        }
        list = &module_manager->dis_list;
    }
    unsigned long long *n = (unsigned long long *)(list + 1);
    if (*n >= 0x43) return false;   // en_list/dis_list capacity
    (*list)[(*n)++] = module;
    return true;
}
FN(void *, get_block_state_id, (Jvm *jvm, void *a1));
FN(jint, get_block_id, (Jvm *jvm, void *a1));
extern "C" bool block_entity_id(Jvm *jvm, void *ent) {
    bool r = false;
    if (ent) {
        void *block_state_id = get_block_state_id(jvm, ent);
        if (block_state_id) r = get_block_id(jvm, block_state_id) != 0;
    }
    SHARED_EPILOG();
    return r;
}

// ---- gap-scan functions of the manager block ----
// sort comparator: descending by the int in slot `module_manager_count` (slots are 16 bytes, starting at +0x50)
struct SortSlot { int v; int padding_04[3]; };
struct SortRec { char padding_00[0x50]; SortSlot k[1]; };
extern "C" int compare_module_slots(const void *pa, const void *pb) {
    SortRec **a = (SortRec **)pa, **b = (SortRec **)pb;
    SortRec *rb = *b;
    unsigned i = module_manager_count;
    return rb->k[i].v - (*a)->k[i].v;
}

// module manager destructor: free every list the constructor (create_module_manager) allocated
FN(void, libc_free, (void *ptr));
#define RELEASE(p)         \
    do {                   \
        if (p) {           \
            libc_free(p);  \
            p = 0;         \
        }                  \
    } while (0)
extern "C" void destroy_module_manager(Mgr *manager) {
    RELEASE(manager->mods);
    for (int i = 0; i < 5; i++) RELEASE(manager->cat[i].list);
#pragma clang loop unroll(disable)
    for (int i = 0; i < 27; i++) RELEASE(manager->reg[i].list);
#pragma clang loop unroll(disable)
    for (int i = 0; i < 27; i++) RELEASE(manager->lists[i]);
    RELEASE(manager->en_list);
    RELEASE(manager->dis_list);
}

// Minecraft formatting codes in a (modified UTF-8) name: returns the last colour code (0-9, a-f; 'r' resets to 'f')
// seen before the first visible character (letter, digit or '_'); *hasText tells whether there was one.
// "§" arrives as the byte pair C2 A7.
FN(unsigned long long, libc_strlen, (const char *str));
extern "C" int last_text_color_code(const char *s, int colour, bool *hasText) {
    *hasText = false;
    unsigned long long n = libc_strlen(s);
    bool sawC2 = false, afterSign = false;
    for (unsigned long long i = 0; i != n; i++) {
        signed char c = s[i];
        if (afterSign) {
            signed char code = c == 'r' ? 'f' : c;
            if ((unsigned short)(c - '0') < 10) colour = code;
            if ((unsigned short)(c - 'a') < 6) colour = code;
            if (c == 'r') colour = code;
            afterSign = false;
        } else if (sawC2) {
            if ((unsigned char)c == 0xa7) {
                afterSign = true;
                sawC2 = false;
            } else {
                sawC2 = (unsigned char)c == 0xc2;
            }
        } else {
            if ((unsigned char)c == 0xc2) {
                sawC2 = true;
                continue;
            }
            if (c == '_' || (unsigned short)(c - '0') < 10 || (unsigned short)((c & 0xdf) - 'A') <= 25) {
                *hasText = true;
                return colour;
            }
        }
    }
    return colour;
}