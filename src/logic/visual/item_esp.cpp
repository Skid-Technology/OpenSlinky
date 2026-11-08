// Item ESP - highlight dropped items through walls.
//
// Every tick the module collects the dropped items within "range" whose id is in the item
// list (or all, with "highlight_all"), at most 100. Each gets a box in the colour set for it.
// With "merge_stacks_labels", stacks of the same item lying together are counted as one and
// labelled with the total ("12").
//
// Callbacks:
//   item_esp_shutdown  module unloaded: free the labels and the item list
//   item_esp_tick      every tick: collect the items
//   item_esp_render    every frame: boxes, then the count labels
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Color3 { float r, g, b; };
struct Listed { int id; int padding_04; Color3 color; int padding_14; };   // 0x18 bytes, sorted by id
struct Drop {                       // 0x58 bytes at 0x302140
    Vec3 prev, pos;
    Color3 color;
    int padding_3c;
    void *ref;                      // global ref to the count label
    int width;
    int id;
    unsigned char count;
};
struct TickEvt { char padding_00[8]; Ctx *ctx; };
struct RenderEvt { char padding_00[8]; Vec3 cam; Ctx *ctx; float partial; unsigned char shader; };

extern "C" unsigned long long item_esp_ref_count;   // count
extern "C" Drop item_esp_refs[100];
extern "C" char item_esp_label_buffer[5];              // label buffer
extern "C" Listed item_esp_default_color_setting;               // colour for unlisted items
extern "C" float item_esp_range;                // range
extern "C" unsigned char item_esp_highlight_all;        // show unlisted items
extern "C" unsigned char item_esp_merge_stacks_labels;        // merge stacks + labels
static const double BOX_RAISE = 0.3;            // box centre above the item's position
static const float BOX_SIZE = 0.3f;             // the unit box (-1..1) scaled to 0.6 blocks
static const float BOX_ALPHA = 0.4f;
static const float LABEL_SCALE = -0.05f;        // text pixels -> blocks (negative: flip upright)
static const double MERGE_DISTANCE_SQ = 0.0625; // stacks within 0.25 blocks count as one
S(str_u_in_item_esp);   // "%u"
extern "C" void (**ptr_to_pGlPushMatrix)();                                 // glPushMatrix
extern "C" void (**ptr_to_pGlPopMatrix)();                                 // glPopMatrix
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);              // glTranslatef
extern "C" void (**ptr_to_pGlScalef)(float, float, float);              // glScalef
extern "C" void (**ptr_to_pGlRotatef)(float, float, float, float);       // glRotatef
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);              // glColor3f
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);       // glColor4f
extern "C" void (**ptr_to_pGlEnable)(unsigned);                         // glEnable
extern "C" void (**ptr_to_pGlDisable)(unsigned);                         // glDisable

FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void *, java_get_world_loaded_entity_list, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void *, java_jni_is_instance_of_entity_item, (void *jvm, void *candidate));
FN(unsigned char, java_get_entity_is_dead, (void *jvm, void *entity));
FN(void *, java_call_entity_item_get_entity_item, (void *jvm, void *entity_item));
FN(int, java_get_item_stack_stack_size, (void *jvm, void *stack));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(void *, libc_bsearch, (const void *key, const void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, compare_name_key, (const void *left, const void *right));
FN(void *, java_get_minecraft_render_manager, (void *jvm, void *minecraft));
FN(void *, java_get_render_manager_text_renderer, (void *jvm, void *minecraft));
FN(float, java_get_render_manager_player_view_x, (void *jvm, void *minecraft));
FN(float, java_get_render_manager_player_view_y, (void *jvm, void *minecraft));
FN(int, java_call_font_renderer_get_string_width, (void *jvm, void *font, void *str));
FN(void, java_font_draw_string, (void *jvm, void *font, void *str, float x, float y, unsigned color, unsigned char shadow));
FN(void, gl_bind_texture, (void *jvm));
FN(void, gl_clear_color, (void *jvm));
FN(void, gl_set_color, (void *jvm, float r, float g, float b, float al));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(void, gl_array_xyz_float, ());
FN(void, gl_array_color1, ());
FN(void, disable_vertex_array_state, ());

typedef void *(*NewGlobalRef)(void *env, void *o);

extern "C" unsigned long long name_table_count, name_table_capacity;
extern "C" void *name_table_entries;

FN(void, libc_free, (void *ptr));

extern "C" void item_esp_shutdown(Hook *hook) {
    if (item_esp_ref_count && hook->ctx && hook->ctx->jvm) {
        JNIEnv *env = ((Jvm *)hook->ctx->jvm)->env;
        for (unsigned long long i = 0; i < item_esp_ref_count; i++) {
            if (!item_esp_refs[i].ref)
                continue;
            (*env)->DeleteGlobalRef(env, item_esp_refs[i].ref);
            item_esp_refs[i].ref = nullptr;
        }
    }
    item_esp_ref_count = 0;
    name_table_count = name_table_capacity = 0;
    libc_free(name_table_entries);
    name_table_entries = nullptr;
}

extern "C" void item_esp_tick(TickEvt *h) {
    Ctx *ctx = h->ctx;
    if (item_esp_ref_count) {
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        for (unsigned long long i = 0; i < item_esp_ref_count; i++) {
            if (!item_esp_refs[i].ref)
                continue;
            (*env)->DeleteGlobalRef(env, item_esp_refs[i].ref);
            item_esp_refs[i].ref = nullptr;
        }
    }
    item_esp_ref_count = 0;
    if (!ctx->world)
        return;
    Vec3 position = java_get_entity_pos_x(ctx->jvm, ctx->player);
    void *list = java_get_world_loaded_entity_list(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, list); i++) {
        void *item = java_jni_is_instance_of_entity_item(ctx->jvm, java_call_list_get(ctx->jvm, list, i));
        if (!item || java_get_entity_is_dead(ctx->jvm, item))
            continue;
        Vec3 pos = java_get_entity_pos_x(ctx->jvm, item);
        if (vec3_distance_sq(pos, position) > (double)(item_esp_range * item_esp_range))
            continue;
        void *stack = java_call_entity_item_get_entity_item(ctx->jvm, item);
        if (!stack)
            continue;
        int count = java_get_item_stack_stack_size(ctx->jvm, stack);
        if (count <= 0)
            continue;
        void *item_stack_get_item = java_call_item_stack_get_item(ctx->jvm, stack);
        if (!item_stack_get_item)
            continue;
        int item_id = java_call_static_item_get_id_from_item(ctx->jvm, item_stack_get_item);
        Listed *l = name_table_entries ? (Listed *)libc_bsearch(&item_id, name_table_entries, name_table_count, 0x18, compare_name_key) : nullptr;
        if (!l) {
            if (!item_esp_highlight_all)
                continue;
            l = &item_esp_default_color_setting;
        }
        Drop *d = &item_esp_refs[item_esp_ref_count];
        d->color = l->color;
        d->id = item_id;
        d->count = (unsigned char)count;
        d->pos = pos;
        d->prev = java_get_entity_prev_pos_x(ctx->jvm, item);
        if (++item_esp_ref_count == 100)
            break;
    }
    if (item_esp_merge_stacks_labels != 1 || !item_esp_ref_count)
        return;
    void *font = java_get_render_manager_text_renderer(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    if (!font)
        return;
    for (unsigned long long i = 0; i < item_esp_ref_count; i++) {
        Drop *d = &item_esp_refs[i];
        unsigned long long n = d->count;
        if (!n)
            continue;
        for (unsigned long long j = 0; j < item_esp_ref_count; j++) {
            Drop *o = &item_esp_refs[j];
            if (i == j || d->id != o->id)
                continue;
            if (MERGE_DISTANCE_SQ > vec3_distance_sq(d->pos, o->pos)) {
                n += o->count;
                o->count = 0;
            }
        }
        unsigned long long shown = n < 99 ? n : 99;   // label at most "99"
        d->count = (unsigned char)shown;
        if (n < 2) {
            d->ref = nullptr;
            continue;
        }
        log_fmt(item_esp_label_buffer, 5, str_u_in_item_esp, (unsigned)shown);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        void *str = (*env)->NewStringUTF(env, item_esp_label_buffer);
        if (!str) {
            d->ref = nullptr;
            continue;
        }
        d->ref = (*env)->NewGlobalRef(env, str);
        (*env)->DeleteLocalRef(env, str);
        if (d->ref)
            d->width = java_call_font_renderer_get_string_width(ctx->jvm, font, d->ref);
    }
}

extern "C" void item_esp_render(RenderEvt *h) {
    if (!item_esp_ref_count)
        return;
    (*ptr_to_pGlEnable)(0xbe2);   // GL_BLEND
    double t = h->partial;
    gl_array_xyz_float();
    for (unsigned long long i = 0; i < item_esp_ref_count; i++) {
        Drop *d = &item_esp_refs[i];
        Vec3 r = vec3_sub(vec3_lerp(d->prev, d->pos, t), h->cam);
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)((float)r.x, (float)(r.y + BOX_RAISE), (float)r.z);
        (*ptr_to_pGlScalef)(BOX_SIZE, BOX_SIZE, BOX_SIZE);
        (*ptr_to_pGlColor4f)(d->color.r, d->color.g, d->color.b, BOX_ALPHA);
        gl_array_color1();
        (*ptr_to_pGlPopMatrix)();
    }
    disable_vertex_array_state();
    if (item_esp_merge_stacks_labels != 1)
        return;
    Ctx *ctx = h->ctx;
    void *minecraft = java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft);
    void *font = java_get_render_manager_text_renderer(ctx->jvm, minecraft);
    float pitch = java_get_render_manager_player_view_x(ctx->jvm, minecraft);
    float yaw = java_get_render_manager_player_view_y(ctx->jvm, minecraft);
    t = h->partial;
    if (h->shader == 1) {
        gl_bind_texture(ctx->jvm);
        gl_set_color(ctx->jvm, 1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        (*ptr_to_pGlEnable)(0xde1);   // GL_TEXTURE_2D
        (*ptr_to_pGlColor3f)(1.0f, 1.0f, 1.0f);
    }
    yaw = -yaw;
    for (unsigned long long i = 0; i < item_esp_ref_count; i++) {
        Drop *d = &item_esp_refs[i];
        if (!d->ref)
            continue;
        Vec3 r = vec3_sub(vec3_lerp(d->prev, d->pos, t), h->cam);
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)((float)r.x, (float)(r.y + BOX_RAISE), (float)r.z);
        (*ptr_to_pGlRotatef)(yaw, 0.0f, 1.0f, 0.0f);
        (*ptr_to_pGlRotatef)(pitch, 1.0f, 0.0f, 0.0f);
        (*ptr_to_pGlScalef)(LABEL_SCALE, LABEL_SCALE, LABEL_SCALE);
        java_font_draw_string(ctx->jvm, font, d->ref, (float)(-d->width * 0.5), -4.0f, 0xffffffffu, 1);
        (*ptr_to_pGlPopMatrix)();
    }
    if (h->shader)
        gl_clear_color(ctx->jvm);
    else
        (*ptr_to_pGlDisable)(0xde1);
}
