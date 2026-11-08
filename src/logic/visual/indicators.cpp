// Indicators - arrows around the crosshair pointing at incoming projectiles.
//
// Every tick the module collects flying arrows, fireballs and ender pearls (the kinds ticked
// in the settings; optionally only ones coming closer), at most 10, each with a distance label
// ("12m": red under 15 m, yellow under 30 m, white further). Every frame it draws a pulsing
// arrow for each around the screen centre, with the label.
//
// Callbacks:
//   indicators_disable  switched off: free the labels
//   indicators_tick     every tick: collect the projectiles
//   indicators_render   every frame: draw the arrows
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Pair { float x, y; };
struct Vec3 { double x, y, z; };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// One projectile (0x38 bytes).
struct Indicator {
    double x0, z0, x1, z1;          // position this tick / last tick
    int kind;                       // entity type: 10 arrow, 12 fireball, 14 ender pearl
    void *ref;                      // the distance label (JNI global ref to a Java string)
    int width;                      // label width in pixels
    unsigned color;                 // label colour (ARGB)
};
// The same array seen 8 bytes earlier, so that .ref is at +0x30 (used by indicators_disable).
struct Cached { char padding_00[0x30]; void *ref; };

struct TickEvt { char padding_00[8]; Ctx *ctx; };   // +0x08 game context
struct RenderEvt {
    char padding_00[8];
    double cam_x;           // +0x08 camera position
    char padding_10[8];
    double cam_z;           // +0x18
    Ctx *ctx;               // +0x20 game context
    void *screen;           // +0x28 screen size
    char padding_30[0x10];
    float partial;          // +0x40 how far we are between two ticks (0..1)
    unsigned char shader;   // +0x44 1 = a shader pipeline is active
};

// ---- settings ----
extern "C" unsigned char indicators_show_kind_10_12_14, indicators_show_kind_10_12_14_second, indicators_show_kind_10_12_14_third;   // show arrows / fireballs / ender pearls
extern "C" unsigned char indicators_only_approaching_items;   // only projectiles coming closer
extern "C" float indicators_arrow_scale_radius, indicators_arrow_scale_radius_second;   // arrow scale / distance from the centre
extern "C" const float indicators_colour_per_kind_10[], indicators_colour_per_kind_10_second[], indicators_colour_per_kind_10_third[];   // arrow colour per kind (r, g, b; index kind - 10)

// ---- module state ----
extern "C" char indicators_module_descriptor[];
extern "C" unsigned long long indicators_cached_count;
extern "C" Cached indicators_player_refs[];
extern "C" Indicator indicators_secondary_refs[10];
extern "C" char indicators_label_buffer[10];
S(fmt_float_0dp_m);   // "%.0fm"

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlScalef)(float, float, float);
extern "C" void (**ptr_to_pGlRotatef)(float, float, float, float);
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlEnable)(unsigned);
extern "C" void (**ptr_to_pGlDisable)(unsigned);
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);
enum { GL_LINE_SMOOTH = 0xb20, GL_TEXTURE_2D = 0xde1 };
FN(void, gl_array_xy_float2, ());          // set up the arrow shape
FN(void, gl_array_color5, ());             // draw it filled
FN(void, gl_array_color6, ());             // draw its outline
FN(void, disable_vertex_array_state, ());
FN(void, gl_bind_texture, (void *jvm));
FN(void, gl_clear_color, (void *jvm));
FN(void, gl_set_color, (void *jvm, float r, float g, float b, float al));

// ---- game / engine functions ----
FN(void *, java_get_minecraft_render_manager, (void *jvm, void *minecraft));
FN(void *, java_get_render_manager_text_renderer, (void *jvm, void *render_manager));
FN(float, java_get_render_manager_player_view_y, (void *jvm, void *render_manager));   // camera yaw
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void *, java_get_world_loaded_entity_list, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(int, java_call_static_entity_list_get_entity_id, (void *jvm, void *entity_list));   // entity type id
FN(unsigned char, java_get_entity_arrow_in_ground, (void *jvm, void *entity_arrow));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(int, java_call_font_renderer_get_string_width, (void *jvm, void *font, void *str));
FN(void, java_font_draw_string, (void *jvm, void *font, void *str, float x, float y, unsigned color, unsigned char shadow));
FN(Pair, rot_scale, (void *screen, float f));                                          // screen size * f
FN(double, time_now, ());
FN(double, libc_sin, (double x));
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(double, libc_atan2, (double y, double x));
FN(float, wrap_angle, (float deg));                                                   // to -180..180
FN(double, libc_sqrt, (double x));

enum { ENTITY_ARROW = 10, ENTITY_FIREBALL = 12, ENTITY_ENDER_PEARL = 14 };

// On disable: free the labels.
extern "C" void indicators_disable(EvHook *hook) {
    if (hook->on || hook->ev != indicators_module_descriptor)
        return;
    if (indicators_cached_count) {
        JNIEnv *env = ((Jvm *)hook->ctx->jvm)->env;
        for (unsigned long long i = 0; i < indicators_cached_count; i++) {
            void *ref = indicators_player_refs[i].ref;
            if (ref)
                (*env)->DeleteGlobalRef(env, ref);
        }
    }
    indicators_cached_count = 0;
}

// Should this entity get an arrow?
static bool wanted(Ctx *ctx, void *entity, int kind) {
    switch (kind) {
    case ENTITY_ENDER_PEARL:
        return indicators_show_kind_10_12_14_third;
    case ENTITY_FIREBALL:
        return indicators_show_kind_10_12_14_second;
    case ENTITY_ARROW:
        return indicators_show_kind_10_12_14 && !java_get_entity_arrow_in_ground(ctx->jvm, entity);
    default:
        return false;
    }
}

// Every tick: collect the projectiles and make their labels.
extern "C" void indicators_tick(TickEvt *h) {
    Ctx *ctx = h->ctx;

    // Free last tick's labels.
    if (indicators_cached_count) {
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        for (unsigned long long i = 0; i < indicators_cached_count; i++)
            if (indicators_secondary_refs[i].ref)
                (*env)->DeleteGlobalRef(env, indicators_secondary_refs[i].ref);
    }
    indicators_cached_count = 0;
    if (!ctx->world)
        return;
    void *font = java_get_render_manager_text_renderer(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    if (!font)
        return;

    Vec3 my_pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    void *entities = java_get_world_loaded_entity_list(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, entities); i++) {
        void *entity = java_call_list_get(ctx->jvm, entities, i);
        int kind = java_call_static_entity_list_get_entity_id(ctx->jvm, entity);
        if (!wanted(ctx, entity, kind))
            continue;
        Vec3 current = java_get_entity_pos_x(ctx->jvm, entity);
        Vec3 previous = java_get_entity_prev_pos_x(ctx->jvm, entity);
        double distance_sq = vec3_distance_sq(current, my_pos);
        if (indicators_only_approaching_items == 1 && distance_sq > vec3_distance_sq(previous, my_pos))
            continue;   // moving away
        double distance = distance_sq >= 0.0 ? __builtin_sqrt(distance_sq) : libc_sqrt(distance_sq);

        Indicator *indicator = &indicators_secondary_refs[indicators_cached_count++];
        indicator->x0 = current.x;
        indicator->z0 = current.z;
        indicator->x1 = previous.x;
        indicator->z1 = previous.z;
        indicator->kind = kind;

        // The label, e.g. "12m".
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        log_fmt(indicators_label_buffer, 10, fmt_float_0dp_m, distance);
        void *text = (*env)->NewStringUTF(env, indicators_label_buffer);
        if (!text) {
            indicator->ref = nullptr;
        } else {
            indicator->ref = (*env)->NewGlobalRef(env, text);
            (*env)->DeleteLocalRef(env, text);
            if (indicator->ref) {
                indicator->width = java_call_font_renderer_get_string_width(ctx->jvm, font, indicator->ref);
                unsigned near_colour = distance > 15.0 ? 0xffffff00u : 0xffff0000u;   // yellow / red
                indicator->color = distance > 30.0 ? 0xffffffffu : near_colour;      // white when far
            }
        }
        if (indicators_cached_count == 10)
            return;
    }
}

// Every frame: the arrows.
extern "C" void indicators_render(RenderEvt *h) {
    if (!indicators_cached_count)
        return;
    Ctx *ctx = h->ctx;
    float partial = h->partial;
    void *render_manager = java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft);
    void *font = java_get_render_manager_text_renderer(ctx->jvm, render_manager);
    float view_yaw = java_get_render_manager_player_view_y(ctx->jvm, render_manager);
    gl_array_xy_float2();

    Pair centre = rot_scale(h->screen, 0.5f);
    (*ptr_to_pGlPushMatrix)();
    (*ptr_to_pGlTranslatef)(centre.x, centre.y, 0.0f);
    (*ptr_to_pGlLineWidth)(1.0f);
    (*ptr_to_pGlEnable)(GL_LINE_SMOOTH);

    // Pulsing transparency.
    double wave = libc_sin(time_now() * 20.0);
    float pulse = (float)(wave * 0.5 + 0.5);

    for (unsigned long long i = 0; i < indicators_cached_count; i++) {
        Indicator *indicator = &indicators_secondary_refs[i];
        double x = lerp_unclamped_float((float)indicator->x1, (float)indicator->x0, partial);
        double z = lerp_unclamped_float((float)indicator->z1, (float)indicator->z0, partial);
        float yaw = (float)(libc_atan2(z - h->cam_z, x - h->cam_x) * 180.0 / 3.141592653589793 + -90.0);
        float angle = wrap_angle(yaw - view_yaw);

        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlRotatef)(angle, 0.0f, 0.0f, 1.0f);
        float scale = indicators_arrow_scale_radius;
        (*ptr_to_pGlTranslatef)(0.0f, -indicators_arrow_scale_radius_second, 0.0f);
        (*ptr_to_pGlScalef)(scale, scale, scale);

        // The label, upright.
        if (indicator->ref) {
            (*ptr_to_pGlPushMatrix)();
            (*ptr_to_pGlRotatef)(-angle, 0.0f, 0.0f, 1.0f);
            if (h->shader == 1) {
                gl_bind_texture(ctx->jvm);
                gl_set_color(ctx->jvm, 1.0f, 1.0f, 1.0f, 1.0f);
            } else {
                (*ptr_to_pGlEnable)(GL_TEXTURE_2D);
            }
            java_font_draw_string(ctx->jvm, font, indicator->ref, (float)(-indicator->width * 0.5), -5.0f, indicator->color, 1);
            if (h->shader == 1)
                gl_clear_color(ctx->jvm);
            else
                (*ptr_to_pGlDisable)(GL_TEXTURE_2D);
            (*ptr_to_pGlPopMatrix)();
        }

        // The arrow, in the colour for its kind.
        (*ptr_to_pGlTranslatef)(0.0f, -15.0f, 0.0f);
        (*ptr_to_pGlScalef)(5.0f, 5.0f, 5.0f);
        int k = indicator->kind - 10;
        (*ptr_to_pGlColor4f)(indicators_colour_per_kind_10[k], indicators_colour_per_kind_10_second[k], indicators_colour_per_kind_10_third[k], pulse);
        gl_array_color5();
        (*ptr_to_pGlColor4f)(0.0f, 0.0f, 0.0f, pulse);
        gl_array_color6();
        (*ptr_to_pGlPopMatrix)();
    }
    (*ptr_to_pGlPopMatrix)();
    disable_vertex_array_state();
}
