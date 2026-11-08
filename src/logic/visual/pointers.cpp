// Pointers - arrows around the crosshair pointing at players you can't see.
//
// Every tick the module collects the players within "range" that are more than
// "skip_when_within" degrees away from where you look (at most 100). Every frame it draws an
// arrow for each around the screen centre, coloured by distance (near -> far colour), team
// colour, or friend / other colour. "style" tilts the ring of arrows into 3D.
//
// Callbacks:
//   pointers_tick    every tick: collect the players
//   pointers_render  every frame: draw the arrows
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Color3 { float r, g, b; };
struct Pair { float x, y; };

// One arrow (0x38 bytes).
struct Pointer {
    double x, z, px, pz;            // position this tick / last tick
    Color3 color;
    float prev_dist, dist;          // distance last tick / this tick
    int padding_34;
};

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
};

// ---- settings ----
extern "C" float pointers_range;
extern "C" float pointers_skip_when_within_this_many;   // degrees from the view direction
extern "C" unsigned char pointers_hide_friendlies;      // also skip friends and teammates
extern "C" int pointers_color_mode;                     // 0 distance, 1 team colour, 2 friend / other
extern "C" Color3 pointers_friend_other_colour, pointers_friend_other_colour_second;   // friend / other colour
extern "C" int pointers_style;                          // non-zero: tilt into 3D
extern "C" float pointers_gradient_distance_range, pointers_gradient_distance_range_second;   // distance for the near / far colour
extern "C" float pointers_size;
extern "C" float pointers_radius;                       // distance from the screen centre
extern "C" unsigned char pointers_pulse_radius_with_distance;   // further players: arrow further out
extern "C" float pointers_near_colour, pointers_near_colour_second, pointers_near_colour_third;   // (r, g, b)
extern "C" float pointers_far_colour, pointers_far_colour_second, pointers_far_colour_third;

// ---- module state ----
extern "C" unsigned long long pointer_count;
extern "C" Pointer pointer_entries[100];

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlScalef)(float, float, float);
extern "C" void (**ptr_to_pGlRotatef)(float, float, float, float);
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);
extern "C" void (**ptr_to_pGlEnable)(unsigned);
enum { GL_LINE_SMOOTH = 0xb20 };
FN(void, gl_array_xy_float2, ());          // set up the arrow shape
FN(void, gl_array_color5, ());             // draw it filled
FN(void, gl_array_color6, ());             // draw its outline
FN(void, disable_vertex_array_state, ());

// ---- game / engine functions ----
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void *, java_get_minecraft_render_manager, (void *jvm, void *minecraft));
FN(float, java_get_render_manager_player_view_x, (void *jvm, void *minecraft));   // camera pitch
FN(float, java_get_render_manager_player_view_y, (void *jvm, void *minecraft));   // camera yaw
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(unsigned char, java_get_entity_is_dead, (void *jvm, void *entity));
FN(int, entity_flags_get, (void *jvm, void *entity));                            // 1/4 friend or teammate, 2 bot
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(double, vec3_distance, (Vec3 a, Vec3 b));
FN(double, libc_atan2, (double y, double x));
FN(float, wrap_angle, (float deg));                                              // to -180..180
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(float, limit_rotation_delta, (float lo, float hi, float v));                  // clamp
FN(float, libc_sqrtf, (float x));
FN(int, entity_name_colour, (void *jvm, void *entity));
FN(Color3, minecraft_color_code_to_rgb, (int team));
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, compare_by_hurt_time_desc, (const void *left, const void *right));
FN(Pair, rot_scale, (void *screen, float f));                                     // screen size * f

// Direction (yaw, degrees) from one point to another, like Minecraft's yaw.
static float yaw_towards(double dz, double dx) {
    return (float)(libc_atan2(dz, dx) * 180.0 / 3.141592653589793 + -90.0);
}

// Every tick: collect the players to point at.
extern "C" void pointers_tick(TickEvt *h) {
    Ctx *ctx = h->ctx;
    pointer_count = 0;
    if (!ctx->world)
        return;
    Vec3 my_pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Vec3 my_prev = java_get_entity_prev_pos_x(ctx->jvm, ctx->player);
    float view_yaw = java_get_render_manager_player_view_y(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    int skip_flags = pointers_hide_friendlies * 5 + 2;   // bots; with "hide_friendlies" also friends/teammates
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        if ((*env)->IsSameObject(env, ctx->player, other))
            continue;
        if (java_get_entity_is_dead(ctx->jvm, other))
            continue;
        int flags = entity_flags_get(ctx->jvm, other);
        if (flags & skip_flags)
            continue;
        Vec3 pos = java_get_entity_pos_x(ctx->jvm, other);
        double distance_sq = vec3_distance_sq(pos, my_pos);
        if (distance_sq > (double)(pointers_range * pointers_range))
            continue;
        float off_view = __builtin_fabsf(wrap_angle(yaw_towards(pos.z - my_pos.z, pos.x - my_pos.x) - view_yaw));
        if (pointers_skip_when_within_this_many > off_view)
            continue;   // already on screen

        Vec3 prev = java_get_entity_prev_pos_x(ctx->jvm, other);
        Pointer *p = &pointer_entries[pointer_count++];
        p->x = pos.x;
        p->z = pos.z;
        p->px = prev.x;
        p->pz = prev.z;
        float distance_sq_f = (float)distance_sq;
        p->dist = distance_sq_f >= 0.0f ? __builtin_sqrtf(distance_sq_f) : libc_sqrtf(distance_sq_f);
        p->prev_dist = (float)vec3_distance(my_prev, prev);
        if (pointers_color_mode == 2)
            p->color = (flags & 5) ? pointers_friend_other_colour : pointers_friend_other_colour_second;
        else if (pointers_color_mode == 1)
            p->color = minecraft_color_code_to_rgb(entity_name_colour(ctx->jvm, other));
        if (pointer_count == 100)
            break;
    }
    libc_qsort(pointer_entries, pointer_count, 0x38, compare_by_hurt_time_desc);
}

// Every frame: the arrows.
extern "C" void pointers_render(RenderEvt *h) {
    if (!pointer_count)
        return;
    Ctx *ctx = h->ctx;
    float partial = h->partial;
    void *render_manager = java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft);
    float view_pitch = java_get_render_manager_player_view_x(ctx->jvm, render_manager);
    float view_yaw = java_get_render_manager_player_view_y(ctx->jvm, render_manager);
    gl_array_xy_float2();

    // Around the screen centre (tilted with the view in 3D style).
    Pair centre = rot_scale(h->screen, 0.5f);
    (*ptr_to_pGlPushMatrix)();
    (*ptr_to_pGlTranslatef)(centre.x, centre.y, 0.0f);
    if (pointers_style) {
        float pitch = 30.0f > view_pitch ? 30.0f : view_pitch;
        (*ptr_to_pGlRotatef)(90.0f - pitch, 1.0f, 0.0f, 0.0f);
    }
    (*ptr_to_pGlLineWidth)(1.0f);
    (*ptr_to_pGlEnable)(GL_LINE_SMOOTH);

    float gradient_range = pointers_gradient_distance_range_second - pointers_gradient_distance_range;
    if (gradient_range == 0.0f)
        gradient_range = 1e-05f;

    for (unsigned long long i = 0; i < pointer_count; i++) {
        Pointer *p = &pointer_entries[i];
        double x = lerp_unclamped_float((float)p->px, (float)p->x, partial);
        double z = lerp_unclamped_float((float)p->pz, (float)p->z, partial);
        float angle = wrap_angle(yaw_towards(z - h->cam_z, x - h->cam_x) - view_yaw);

        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlRotatef)(angle, 0.0f, 0.0f, 1.0f);
        float scale = pointers_size * 10.0f;
        (*ptr_to_pGlScalef)(scale, scale, scale);
        // 0 at the near distance .. 1 at the far distance.
        float distance = lerp_unclamped_float(p->prev_dist, p->dist, partial);
        float far = limit_rotation_delta(0.0f, 1.0f, (distance - pointers_gradient_distance_range) / gradient_range);
        float radius = pointers_radius;
        if (pointers_pulse_radius_with_distance)
            radius *= far + 1.0f;
        (*ptr_to_pGlTranslatef)(0.0f, -radius, 0.0f);
        if (pointers_color_mode) {
            (*ptr_to_pGlColor3f)(p->color.r, p->color.g, p->color.b);
        } else {
            float r = lerp_unclamped_float(pointers_near_colour, pointers_far_colour, far);
            float g = lerp_unclamped_float(pointers_near_colour_second, pointers_far_colour_second, far);
            float b = lerp_unclamped_float(pointers_near_colour_third, pointers_far_colour_third, far);
            (*ptr_to_pGlColor3f)(r, g, b);
        }
        gl_array_color5();
        (*ptr_to_pGlColor3f)(0.0f, 0.0f, 0.0f);   // black outline
        gl_array_color6();
        (*ptr_to_pGlPopMatrix)();
    }
    (*ptr_to_pGlPopMatrix)();
    disable_vertex_array_state();
}
