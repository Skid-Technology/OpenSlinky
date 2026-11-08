// 0xd000: ImGui widget wrappers (mark profile dirty on change), bounded string builder
#include "slinky/engine.h"
#include <stdarg.h>
FN(void, set_profile_dirty, (unsigned char v));
FN(unsigned char, draw_dropdown_row, (void *gui, void *window, void *label, void *setting, int count, void *names, void *value, unsigned char optional));
FN(unsigned char, draw_color_swatch_row, (void *gui, void *window, void *label, void *color));
FN(unsigned char, draw_list_editor_row, (void *gui, void *window, void *label, void *popup_title, void *char_filter, int kind, void *list_ops,
                                         void *candidates, void *candidate_count));

// enum setting: label, option count, option names, current value (+ saved copy); `optional` adds a "none" entry with a clear cross
struct DropdownSetting { char padding_00[0x18]; void *label; int count; int padding_24; void *names; char value_and_saved[8]; unsigned char optional; };
extern "C" void widget_enum(void *window, void *gui, DropdownSetting *setting) {
    if (draw_dropdown_row(gui, window, setting->label, setting, setting->count, setting->names, setting->value_and_saved, setting->optional)) set_profile_dirty(1);
}
// colour setting: label, then the colour (current + saved)
struct ColorSetting { void *label; char color[8]; };
extern "C" void widget_color(void *window, void *gui, ColorSetting *setting) {
    if (draw_color_swatch_row(gui, window, setting->label, setting->color)) set_profile_dirty(1);
}
// list setting: label, "add" popup title and character filter, entry kind (0 names, 1 names + colour), list callbacks,
// and an optional provider of the candidate items offered by the "add" popup
struct ListSetting {
    void *label, *popup_title, *char_filter; int kind; int padding_1c; char list_ops[0x38];
    void (*get_candidates)(void **items, void **count); unsigned char changed;
};
extern "C" void widget_items(void *window, void *gui, ListSetting *setting) {
    void *candidates = 0, *candidate_count = 0;
    if (setting->get_candidates) setting->get_candidates(&candidates, &candidate_count);
    if (draw_list_editor_row(gui, window, setting->label, setting->popup_title, setting->char_filter, setting->kind, setting->list_ops, candidates, candidate_count)) {
        setting->changed = 1;
        set_profile_dirty(1);
    }
}

struct Buf { char *p; unsigned long long cap, len; };
FN(unsigned long long, libc_strlen, (const char *str));
FN(void *, libc_memcpy_alt, (void *dest, const void *src, unsigned long long n));
FN(int, libc_vsnprintf, (char *buf, unsigned long long n, const char *fmt, va_list ap));   // vsnprintf
extern "C" void buffer_init(Buf *b, char *p, unsigned long long cap) { b->p = p; b->cap = cap; b->len = 0; }
extern "C" bool buffer_append(Buf *b, const char *s) {
    unsigned long long len = b->len;
    if (b->cap <= len) return false;
    unsigned long long avail = b->cap - len;
    unsigned long long n = libc_strlen(s);
    if (n >= avail) return false;
    libc_memcpy_alt(b->p + len, s, n);
    b->len += n;
    return true;
}
extern "C" bool buffer_printf(Buf *b, const char *fmt, ...) {
    if (b->cap <= b->len) return false;
    unsigned long long avail = b->cap - b->len;
    va_list ap;
    va_start(ap, fmt);
    int result = libc_vsnprintf(b->p + b->len, avail, fmt, ap);
    unsigned long long nl;
    bool ok;
    if (result < 0 || avail <= (unsigned)result) { nl = b->cap; ok = false; }
    else { nl = (unsigned)result + b->len; ok = true; }
    b->len = nl;
    return ok;
}
extern "C" bool buffer_push_char(Buf *b, char character) {
    unsigned long long len = b->len, cap = b->cap;
    if (len < cap) {
        char *p = b->p;
        b->len = len + 1;
        p[len] = character;
    }
    return len < cap;
}
extern "C" bool buffer_terminate(Buf *b) {
    unsigned long long len = b->len, cap = b->cap;
    if (len < cap) {
        b->p[len] = 0;
        b->len++;
    }
    return len < cap;
}

// ---- generic dynamic array: data pointer + {u32 cap, u32 count} header, element size passed in ----
struct AHdr { unsigned cap, count; };
typedef int (*ACmp)(void *ctx, const void *key, const void *elem);
FN(void *, libc_realloc, (void *ptr, unsigned long long n));   // realloc
extern "C" __attribute__((noreturn)) void libc_abort();   // abort
FN(void *, libc_memmove, (void *dest, const void *src, unsigned long long n));   // memmove
extern "C" void *element_at(void *base, unsigned elem, AHdr *, unsigned i) { return (char *)base + elem * i; }
static inline __attribute__((always_inline)) void agrow(void **data, unsigned elem, AHdr *header) {
    if (header->cap > 0x7fffffffu) libc_abort();
    unsigned cap = header->cap ? header->cap * 2 : 8;
    if (elem && cap > 0xffffffffu / elem) libc_abort();
    header->cap = cap;
    void *p = libc_realloc(*data, (unsigned long long)cap * elem);
    if (!p) libc_abort();
    *data = p;
}
// push_back
extern "C" void *vector_push_back(void **data, unsigned elem, AHdr *header, const void *src) {
    if (header->count >= header->cap) agrow(data, elem, header);
    unsigned i = header->count;
    header->count = i + 1;
    return libc_memcpy_alt((char *)*data + i * elem, src, elem);
}
// insert at index (shares the manifest range of vector_push_back)
extern "C" void *insert_index(void **data, unsigned elem, AHdr *header, const void *src, unsigned idx) {
    if (idx > header->count) return 0;
    if (header->count >= header->cap) agrow(data, elem, header);
    char *base = (char *)*data;
    char *at = base + idx * elem;
    libc_memmove(base + (idx + 1) * elem, at, (header->count - idx) * elem);
    libc_memcpy_alt(at, src, elem);
    header->count++;
    return at;
}
// binary search
extern "C" void *binary_search(void *base, unsigned elem, AHdr *header, const void *key, ACmp cmp) {
    unsigned hi = header->count;
    if (!hi) return 0;
    unsigned lo = 0;
    do {
        unsigned mid = lo + ((hi - lo) >> 1);
        char *p = (char *)base + mid * elem;
        int result = cmp(0, key, p);
        if (result < 0) hi = mid;
        else if (result == 0) return p;
        else lo = mid + 1;
    } while (lo < hi);
    return 0;
}
// sorted insert (returns the new element)
extern "C" void *sorted_insert(void **data, unsigned elem, AHdr *header, const void *key, ACmp cmp) {
    unsigned hi = header->count, idx;
    if (!hi) {
        idx = 0;
    } else {
        char *base = (char *)*data;
        unsigned lo = 0;
        for (;;) {
            unsigned mid = lo + ((hi - lo) >> 1);
            int result = cmp(0, key, base + mid * elem);
            if (result < 0) {
                hi = mid;
                if (lo >= hi) break;
            } else if (result == 0) {
                hi = mid;
                break;
            } else {
                lo = mid + 1;
                if (lo >= hi) break;
            }
        }
        idx = hi;
    }
    if (header->count >= header->cap) agrow(data, elem, header);
    char *base = (char *)*data;
    char *at = base + idx * elem;
    libc_memmove(base + (idx + 1) * elem, at, (header->count - idx) * elem);
    libc_memcpy_alt(at, key, elem);
    header->count++;
    return at;
}

// ---- install / remove "slinky_handler" in the client's netty pipeline ----
#include "slinky/jids.h"
FN(void *, get_minecraft, (Jvm *jvm));
FN(void *, java_call_minecraft_get_net_handler, (Jvm *jvm, void *minecraft));
FN(void *, java_get_net_handler_play_client_net_manager, (Jvm *jvm, void *net_handler_play_client));
FN(void *, java_get_network_manager, (Jvm *jvm, void *network_manager));
FN(void *, java_call_channel_pipeline, (Jvm *jvm, void *channel_pipeline));
FN(void *, java_call_channel_pipeline_get, (Jvm *jvm, void *pipe, void *cls));
FN(void, java_call_channel_pipeline_add_before, (Jvm *jvm, void *pipe, const char *before, const char *name, jobject h));
FN(void, java_call_channel_pipeline_remove, (Jvm *jvm, void *pipe, void *cls));
extern "C" const char str_packet_handler[], str_slinky_handler[];
struct HClass { jclass cls; jmethodID ctor; };
static inline __attribute__((always_inline)) int pipeline(Jvm *jvm, void **out) {
    void *minecraft = get_minecraft(jvm);
    if (!minecraft) return 0x3fc;
    void *minecraft_get_net_handler = java_call_minecraft_get_net_handler(jvm, minecraft);
    if (!minecraft_get_net_handler) return 0x3fd;
    void *network_manager = java_get_network_manager(jvm, java_get_net_handler_play_client_net_manager(jvm, minecraft_get_net_handler));
    if (!network_manager) return 0x3fd;
    void *pipe = java_call_channel_pipeline(jvm, network_manager);
    if (!pipe) return 0x3fd;
    *out = pipe;
    return 0;
}
extern "C" int register_channel_class(Jvm *jvm, HClass *h) {
    void *pipe;
    int error_code = pipeline(jvm, &pipe);
    if (error_code) return error_code;
    if (java_call_channel_pipeline_get(jvm, pipe, h->cls)) return 0;
    JNIEnv *env = jvm->env;
    jobject object = (*env)->NewObject(env, h->cls, h->ctor);
    if (!object) return 0x7d5;
    java_call_channel_pipeline_add_before(jvm, pipe, str_packet_handler, str_slinky_handler, object);
    return (*env)->ExceptionCheck(env) ? 0x7d4 : 0;
}
extern "C" int unregister_channel_class(Jvm *jvm, HClass *h) {
    void *pipe;
    int error_code = pipeline(jvm, &pipe);
    if (error_code) return error_code;
    if (!java_call_channel_pipeline_get(jvm, pipe, h->cls)) return 0;
    JNIEnv *env = jvm->env;
    java_call_channel_pipeline_remove(jvm, pipe, h->cls);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return 0x7d4;
    }
    return 0;
}

// ---- reach: entity mouse-over with the module's reach / hitbox expansion (piercing, ghost hand aware) ----
struct V3d { double x, y, z; };
struct Box6 { double v[6]; };
struct ReachA { Ctx *c; void *padding_08; double reach, expand; };
FN(bool, piercing_should_pierce_blocks_entry, (Ctx *ctx));                                      // piercing: blocks
FN(bool, ghost_hand_should_ignore_entity_entry, (Ctx *ctx, jobject entity));                           // ghost hand: may hit through e
FN(int, piercing_decide_which_entity_entry, (Ctx *ctx, jobject e1, jobject e2, bool inside));  // piercing: 0/1 preferred, 2 none
FN(jobject, java_call_entity_ray_trace, (Jvm *jvm, jobject object, double d, float pt));    // rayTrace
FN(jobject, java_call_entity_living_base_get_position, (Jvm *jvm, jobject entity_living_base));                        // eye position
FN(V3d, java_get_vec3_x_coord, (Jvm *jvm, jobject vec3));
FN(jobject, java_get_moving_object_position_hit_vec, (Jvm *jvm, jobject mop));                      // hitVec
FN(double, vec3_distance, (V3d a, V3d b));
FN(V3d, java_get_entity_rotation_pitch, (Jvm *jvm, jobject entity));                            // look vector
FN(V3d, vec3_scale, (V3d a, double s));
FN(V3d, vec3_add, (V3d a, V3d b));
FN(jobject, java_new_vec3_init, (Jvm *jvm, V3d v));
FN(jobject, java_get_world_loaded_entity_list, (Jvm *jvm, void *world));                          // loaded entity list
FN(int, java_call_list_size, (Jvm *jvm, jobject l));
FN(int, java_get_moving_object_position_type_ordinal, (Jvm *jvm, jobject mop));                          // typeOfHit ordinal
FN(jobject, java_call_list_get, (Jvm *jvm, jobject l, int i));
FN(bool, java_call_entity_can_be_collided_with, (Jvm *jvm, jobject entity));                           // canBeCollidedWith
FN(jobject, java_get_entity_bounding_box, (Jvm *jvm, jobject entity));                        // bounding box
FN(Box6, java_get_axis_aligned_bb_min_x, (Jvm *jvm, jobject bb));
FN(Box6, aabb_expand, (Box6 b, double d));                             // expand
FN(jobject, java_new_axis_aligned_bb_init, (Jvm *jvm, Box6 b));
FN(jobject, java_axis_aligned_bb_calculate_intercept, (Jvm *jvm, jobject bb, jobject from, jobject to));   // calculateIntercept
FN(bool, aabb_contains_point, (Box6 b, V3d p));                                // isVecInside
FN(V3d, aabb_clamp_point, (Box6 b, V3d p));                                 // clamp into box
FN(jobject, java_moving_object_position_init, (Jvm *jvm, jobject moving_object_position, jobject v));             // new MovingObjectPosition
FN(void, java_set_minecraft, (Jvm *jvm, void *minecraft, jobject mop));               // objectMouseOver =
extern "C" const double offset_1_5, tenth;
extern "C" const float one_in_imgui_widgets;
#define RJ ((Jvm *)ctx->jvm)
extern "C" void reach_distance_check(ReachA *a) {
    Ctx *ctx = a->c;
    double reach = a->reach;
    double d = offset_1_5 + reach;
    bool pierce = piercing_should_pierce_blocks_entry(ctx);
    jobject mop = java_call_entity_ray_trace(RJ, ctx->player, d, one_in_imgui_widgets);
    jobject eyes = java_call_entity_living_base_get_position(RJ, ctx->player);
    V3d eye = java_get_vec3_x_coord(RJ, eyes);
    if (!pierce && mop) d = vec3_distance(eye, java_get_vec3_x_coord(RJ, java_get_moving_object_position_hit_vec(RJ, mop)));
    V3d endv = vec3_add(eye, vec3_scale(java_get_entity_rotation_pitch(RJ, ctx->player), d));
    jobject endo = java_new_vec3_init(RJ, endv);
    jobject list = java_get_world_loaded_entity_list(RJ, ctx->world);
    int n = java_call_list_size(RJ, list);
    bool blockHit = mop && java_get_moving_object_position_type_ordinal(RJ, mop) == 1;
    if (n <= 0) {
        if (blockHit) java_set_minecraft(RJ, ctx->minecraft, mop);
        return;
    }
    jobject pointed = 0, hitv = 0;
    double closest = reach;
    bool found = false;
    for (int i = 0; i != n; i++) {
        jobject element = java_call_list_get(RJ, list, i);
        JNIEnv *env = RJ->env;
        if ((*env)->IsSameObject(env, (jobject)ctx->player, element)) continue;
        if (!java_call_entity_can_be_collided_with(RJ, element)) continue;
        Box6 bb = aabb_expand(java_get_axis_aligned_bb_min_x(RJ, java_get_entity_bounding_box(RJ, element)), a->expand);
        jobject ic = java_axis_aligned_bb_calculate_intercept(RJ, java_new_axis_aligned_bb_init(RJ, bb), eyes, endo);
        jobject entity;
        double nd;
        if (aabb_contains_point(bb, eye)) {   // eye inside the (expanded) box
            found = true;
            entity = pointed;
            nd = closest;
            if (closest >= 0.0 && !(pointed && piercing_decide_which_entity_entry(ctx, pointed, element, true) == 1)) {
                hitv = ic ? java_get_moving_object_position_hit_vec(RJ, ic) : eyes;
                entity = element;
                nd = 0.0;
            }
        } else {
            if (!ic) continue;
            jobject moving_object_position = java_get_moving_object_position_hit_vec(RJ, ic);
            double dist = vec3_distance(eye, java_get_vec3_x_coord(RJ, moving_object_position));
            int result = piercing_decide_which_entity_entry(ctx, pointed, element, found);
            bool take = result == 2 ? closest == 0.0 || closest > dist : result == 0 && reach > dist;
            entity = pointed;
            nd = closest;
            if (take) {
                hitv = moving_object_position;
                entity = element;
                nd = dist;
            }
        }
        bool through = ghost_hand_should_ignore_entity_entry(ctx, entity);
        blockHit = blockHit & through;
        closest = nd;
        pointed = entity;
    }
    if (blockHit) {
        java_set_minecraft(RJ, ctx->minecraft, mop);
        return;
    }
    if (!pointed) return;
    if (!(pierce | !mop | !(closest > d))) return;
    Box6 bb = aabb_expand(java_get_axis_aligned_bb_min_x(RJ, java_get_entity_bounding_box(RJ, pointed)), tenth);
    V3d p = aabb_clamp_point(bb, java_get_vec3_x_coord(RJ, hitv));
    java_set_minecraft(RJ, ctx->minecraft, java_moving_object_position_init(RJ, pointed, java_new_vec3_init(RJ, p)));
}
#undef RJ

// ---- counting allocator (the allocation counter at allocation_counter is checked for leaks on unload) ----
FN(void *, libc_malloc, (unsigned long long n));                          // malloc
FN(void, libc_free, (void *ptr));                                         // free
FN(void *, libc_memset, (void *ptr, int c, unsigned long long n));          // memset
extern "C" long long allocation_counter;
extern "C" void *debug_alloc(unsigned long long n) {   // allocate n bytes filled with 0xaa
    void *p = libc_malloc(n);
    if (!p) return 0;
    libc_memset(p, 0xaa, n);
    __atomic_fetch_add(&allocation_counter, 1, __ATOMIC_SEQ_CST);
    return p;
}
extern "C" void debug_free(void *p) {
    if (p) {
        __atomic_fetch_sub(&allocation_counter, 1, __ATOMIC_SEQ_CST);
        libc_free(p);
    }
}

// ---- define the netty channel handler class "SlinkyChannelHandler" from the embedded class bytes ----
// registers its native `onPacket(Object, boolean, long, int)` (implemented by `onPacket`), looks up the
// constructor and keeps a global ref of the class. 0x7d0: no constructor, 0x7d5: no global ref.
FN(int, define_class, (JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jint len, const JNINativeMethod *methods,
               jclass *out));
extern "C" const jbyte *ptr_to_str_str;   // class file bytes of SlinkyChannelHandler
extern "C" const jint *ptr_to_rdata_2b4bf8;    // their length
extern "C" const char str_onpacket[], str_ljava_lang_object_zji_i[], str_slinkychannelhandler[], str_init[], str_v_in_imgui_widgets[];
extern "C" int install_packet_channel(Jvm *jvm, jobject loader, HClass *out, void *onPacket) {
    JNINativeMethod native = {(char *)str_onpacket /* "onPacket" */, (char *)str_ljava_lang_object_zji_i /* "(Ljava/lang/Object;ZJI)I" */,
                              onPacket};
    JNIEnv *env = jvm->env;
    jclass cls;
    int error_code = define_class(env, str_slinkychannelhandler /* "SlinkyChannelHandler" */, loader, ptr_to_str_str, *ptr_to_rdata_2b4bf8, &native, &cls);
    if (error_code) return error_code;
    out->ctor = (*env)->GetMethodID(env, cls, str_init /* "<init>" */, str_v_in_imgui_widgets /* "()V" */);
    if (!out->ctor) return 0x7d0;
    out->cls = (jclass)(*env)->NewGlobalRef(env, cls);
    return out->cls ? 0 : 0x7d5;
}
// drop the class again: unregister its natives and release the global ref
extern "C" void release_packet_channel(JNIEnv *env, jclass *cls) {
    if (!*cls) return;
    (*env)->UnregisterNatives(env, *cls);
    (*env)->DeleteGlobalRef(env, *cls);
    *cls = 0;
}

// grow a dynamic array when it is full (the capacity step of vector_push_back)
extern "C" void grow_dynamic_array_full(void **data, unsigned elem, AHdr *header) {
    if (__builtin_expect(header->count >= header->cap, 0)) agrow(data, elem, header);
}

// calloc / realloc with the same allocation counter
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(void *, libc_realloc, (void *p, unsigned long long n));
extern "C" void *debug_calloc(unsigned long long n, unsigned long long size) {
    void *p = libc_calloc(n, size);
    if (p) __atomic_fetch_add(&allocation_counter, 1, __ATOMIC_SEQ_CST);
    return p;
}
extern "C" void *debug_realloc(void *old, unsigned long long n) {
    bool had = old != 0;
    void *p = libc_realloc(old, n);
    if (!(p == 0 || had)) __atomic_fetch_add(&allocation_counter, 1, __ATOMIC_SEQ_CST);
    return p;
}
