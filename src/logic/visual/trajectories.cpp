// Trajectories - show where your arrow / ender pearl / snowball will land.
//
// Every tick, while holding a bow, ender pearl, snowball or egg (each can be switched off),
// the module simulates the flight (up to 100 ticks, with drag 0.99 and gravity) and finds the
// block or entity it would hit. Every frame it draws the path as a line, coloured by whether
// it hits something, and a box around what it hits.
//
// Callbacks:
//   trajectories_disable  switched off: free the entity lists
//   trajectories_tick     every tick: simulate the flight
//   trajectories_render   every frame: draw the path and the hit box
#include "slinky/engine.h"
#include "slinky/jids.h"
extern "C" void slinky_logf(const char *fmt, ...);

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Box { Vec3 lo, hi; };
struct Point { float x, y, z; };
struct RenderEvt { char padding_00[8]; Vec3 cam; Ctx *ctx; float partial; };

struct EvHook { char padding_00[8]; Ctx *ctx; void *ev; unsigned char on; };

extern "C" char trajectories_module_descriptor[];           // Trajectories module descriptor
extern "C" void *trajectories_entities[1];
extern "C" void *trajectories_boxes[1];

FN(void, libc_free, (void *ptr));

extern "C" unsigned trajectories_tracked_entity_count;         // tracked entity count (header at 0x304480)
extern "C" unsigned trajectories_tracked_box_count;         // tracked box count (header at 0x304490)
extern "C" char trajectories_entity_vector_header[], trajectories_box_vector_header[];
extern "C" int trajectories_path_length;              // path length
extern "C" unsigned char trajectories_hit_colour_fourth;    // hit colour
extern "C" Point trajectories_screen_points[100];
extern "C" unsigned char trajectories_something_hit;    // something hit
extern "C" Box trajectories_hit_box;              // hit box
extern "C" Vec3 trajectories_hit_entity_movement_this_tick;             // hit entity movement this tick
extern "C" float trajectories_gravity;            // gravity
extern "C" float trajectories_launch_power, trajectories_launch_power_second;   // launch power
extern "C" unsigned char trajectories_render_bow;    // bows
extern "C" unsigned char trajectories_uncharged_bow;    // uncharged bow
extern "C" unsigned char trajectories_pearls;    // pearls
extern "C" unsigned char trajectories_throwables;    // throwables
extern "C" float trajectories_line_width;            // line width
extern "C" float trajectories_hit_colour, trajectories_hit_colour_second, trajectories_hit_colour_third;   // hit colour
extern "C" float trajectories_miss_colour, trajectories_miss_colour_second, trajectories_miss_colour_third;   // miss colour
static const double AIR_DRAG = 0.99;           // speed kept per tick
static const double ENTITY_HIT_MARGIN = 0.3;   // projectiles hit entities a bit outside their box
extern "C" void (**ptr_to_pGlPushMatrix)();                                 // glPushMatrix
extern "C" void (**ptr_to_pGlPopMatrix)();                                 // glPopMatrix
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);              // glTranslatef
extern "C" void (**ptr_to_pGlScalef)(float, float, float);              // glScalef
extern "C" void (**ptr_to_pGlLineWidth)(float);                            // glLineWidth
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);              // glColor3f
extern "C" void (**ptr_to_pGlEnable)(unsigned);                         // glEnable
extern "C" void (**ptr_to_pGlEnableClientState)(unsigned);                         // glEnableClientState
extern "C" void (**ptr_to_pGlVertexPointer)(int, unsigned, int, const void *); // glVertexPointer
extern "C" void (**ptr_to_pGlDrawArrays)(unsigned, int, int);               // glDrawArrays

FN(unsigned char, is_silent_rotation_active, ());
FN(void *, get_game_settings, (void *jvm, void *object));
FN(int, java_get_game_settings_third_person_view, (void *jvm, void *game_settings));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(int, java_get_entity_player_item_in_use_count, (void *jvm, void *entity_player));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void, throwable_velocity, (Ctx *ctx, Vec3 *start, Rot rotation, float partial, Vec3 *pos, Vec3 *vel));
FN(void *, java_get_world_loaded_entity_list, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(unsigned char, java_call_entity_can_be_collided_with, (void *jvm, void *entity));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(Box, aabb_expand, (Box b, double grow));
FN(Box, aabb_union, (Vec3 a, Vec3 b));
FN(Box, aabb_make, (Vec3 lo, Vec3 hi));
FN(unsigned char, aabb_intersects, (Box a, Box b));
FN(unsigned char, aabb_ray_slab_test, (Box b, Vec3 origin, Vec3 dir));
FN(void, vector_push_back, (void *vec, unsigned elem, void *hdr, const void *item));
FN(void *, element_at, (void *vec, unsigned elem, void *hdr, unsigned i));
FN(void *, java_new_vec3_init, (void *jvm, Vec3 v));
FN(void *, java_world_ray_trace_blocks, (void *jvm, void *world, void *from, void *to, unsigned char liquids, unsigned char ignore_no_box, unsigned char last));
FN(void *, java_get_moving_object_position_hit_vec, (void *jvm, void *hit));
FN(Vec3, java_get_vec3_x_coord, (void *jvm, void *vec3));
FN(Vec3, java_get_moving_object_position_block_x, (void *jvm, void *hit));
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Vec3, vec3_scale, (Vec3 v, double k));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_reciprocal, (Vec3 v));
FN(void, gl_array_xyz_float, ());
FN(void, gl_array_color2, ());
FN(void, disable_vertex_array_state, ());

typedef unsigned char (*IsSameObject)(void *env, void *a, void *b);

// Arrow speed after drawing the bow for `ticks` ticks (as Minecraft computes it); 0 when too short.
static float bow_power(int ticks) {
    float t = (float)ticks / 20.0f;
    float v = (t * t + (t + t)) / 3.0f;
    if (0.1 > (double)v)
        return 0.0f;
    double p = (double)(1.0f < v ? 1.0f : v) * 1.5;
    return (float)(p + p);
}

extern "C" void trajectories_disable(EvHook *hook) {
    if (!hook->on && hook->ev == trajectories_module_descriptor) {
        libc_free(trajectories_entities[0]);
        trajectories_entities[0] = nullptr;
        __builtin_memset(trajectories_entity_vector_header, 0, 8);   // cap + count
        libc_free(trajectories_boxes[0]);
        trajectories_boxes[0] = nullptr;
        __builtin_memset(trajectories_box_vector_header, 0, 8);      // cap + count
    }
}

extern "C" void trajectories_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    unsigned char pearl, thrown;
    int item_id = -1;
    int why = 0;
    static int last_why;
    if (ctx->in_game != 1) { why = 1; goto none; }
    if (is_silent_rotation_active()) { why = 2; goto none; }
    if (java_get_game_settings_third_person_view(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft))) { why = 3; goto none; }
    {
        void *stack = item_held_hotbar_slot(ctx->jvm, ctx->player);
        if (!stack) { why = 4; goto none; }
        item_id = java_call_static_item_get_id_from_item(ctx->jvm, stack);
        if (item_id == 0x170) {                      // ender pearl
            if (trajectories_pearls != 1) { why = 5; goto none; }
            trajectories_launch_power = 1.5f;
            trajectories_launch_power_second = 1.5f;
            trajectories_gravity = 0.03f;
            pearl = 1;
            thrown = 0;
        } else if (item_id == 0x14c || item_id == 0x158) {   // snowball, egg
            if (trajectories_throwables != 1) { why = 6; goto none; }
            trajectories_launch_power = 1.5f;
            trajectories_launch_power_second = 1.5f;
            trajectories_gravity = 0.03f;
            thrown = 1;
            pearl = 0;
        } else if (item_id == 0x105) {               // bow
            if (trajectories_render_bow != 1) { why = 7; goto none; }
            trajectories_gravity = 0.05f;
            if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player)) {
                int used = java_get_entity_player_item_in_use_count(ctx->jvm, ctx->player);
                float p = bow_power(72000 - used);
                trajectories_launch_power_second = p;
                if (p == 0.0f) { why = 10; goto none; }
                trajectories_launch_power = bow_power(71999 - used);
            } else {
                if (trajectories_uncharged_bow != 1) { why = 8; goto none; }
                trajectories_launch_power = 3.0f;
                trajectories_launch_power_second = 3.0f;
            }
            thrown = 1;
            pearl = 0;
        } else {
            why = 9; goto none;
        }
    }
    {
        static int sim_logged;
        if (!sim_logged) { sim_logged = 1; slinky_logf("traj: simulating item=%d pearl=%d thrown=%d", item_id, pearl, thrown); }
        Rot rot = get_entity_rotation(ctx->jvm, ctx->player);
        Vec3 position = java_get_entity_pos_x(ctx->jvm, ctx->player);
        Vec3 pt, vel;
        throwable_velocity(ctx, &position, rot, 0.0f, &pt, &vel);
        trajectories_tracked_entity_count = 0;
        trajectories_tracked_box_count = 0;
        // Entities that could be in the way (in a big box along the throw direction).
        void *list = java_get_world_loaded_entity_list(ctx->jvm, ctx->world);
        Vec3 lo = {pt.x, -100.0, pt.z};
        Vec3 hi = {vel.x * 1000.0 + pt.x, pt.y + 200.0, 1000.0 * vel.z + pt.z};
        Box area = aabb_union(lo, hi);
        for (int i = 0; i < java_call_list_size(ctx->jvm, list); i++) {
            void *element = java_call_list_get(ctx->jvm, list, i);
            JNIEnv *env = ((Jvm *)ctx->jvm)->env;
            if ((*env)->IsSameObject(env, element, ctx->player))
                continue;
            if (!java_call_entity_can_be_collided_with(ctx->jvm, element))
                continue;
            Box bb = aabb_expand(java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, element)), ENTITY_HIT_MARGIN);
            if (!aabb_intersects(bb, area))
                continue;
            vector_push_back(trajectories_entities, 8, trajectories_entity_vector_header, &element);
            vector_push_back(trajectories_boxes, 0x30, trajectories_box_vector_header, &bb);
        }
        // Fly tick by tick: stop at the first block or entity hit.
        for (int n = 0; n < 100; n++) {
            Vec3 from = pt;
            pt = vec3_add(pt, vel);
            void *hit = java_world_ray_trace_blocks(ctx->jvm, ctx->world, java_new_vec3_init(ctx->jvm, from), java_new_vec3_init(ctx->jvm, pt), 0, 1, 0);
            if (hit) {
                trajectories_path_length = n + 1;
                trajectories_something_hit = 1;
                trajectories_hit_colour_fourth = pearl;
                pt = java_get_vec3_x_coord(ctx->jvm, java_get_moving_object_position_hit_vec(ctx->jvm, hit));
                vel = vec3_sub(pt, from);
                Vec3 block = java_get_moving_object_position_block_x(ctx->jvm, hit);
                trajectories_hit_box = aabb_make(block, vec3_add(block, vec3_make(1.0, 1.0, 1.0)));
                trajectories_hit_entity_movement_this_tick = vec3_make(0.0, 0.0, 0.0);
            }
            Vec3 dir = vec3_reciprocal(vel);
            for (unsigned j = 0; j < trajectories_tracked_box_count; j++) {
                Box b = *(Box *)element_at(trajectories_boxes[0], 0x30, trajectories_box_vector_header, j);
                if (!aabb_ray_slab_test(b, from, dir))
                    continue;
                trajectories_path_length = n + 1;
                trajectories_something_hit = 1;
                trajectories_hit_colour_fourth = thrown;
                trajectories_hit_box = b;
                void *entity = *(void **)element_at(trajectories_entities[0], 8, trajectories_entity_vector_header, j);
                trajectories_hit_entity_movement_this_tick = vec3_sub(java_get_entity_pos_x(ctx->jvm, entity), java_get_entity_prev_pos_x(ctx->jvm, entity));
                return;
            }
            if (hit)
                return;
            vel = vec3_scale(vel, AIR_DRAG);
            vel.y -= trajectories_gravity;
        }
        trajectories_path_length = 100;
        trajectories_something_hit = 0;
        trajectories_hit_colour_fourth = 0;
    }
    return;
none:
    if (why != last_why) {
        last_why = why;
        slinky_logf("traj: tick idle why=%d item=%d", why, item_id);
    }
    trajectories_path_length = 0;
}

extern "C" void trajectories_render(RenderEvt *h) {
    static int render_logged;
    if (!render_logged) {
        render_logged = 1;
        slinky_logf("traj: first render call path=%d", trajectories_path_length);
    }
    if (!trajectories_path_length)
        return;
    Ctx *ctx = h->ctx;
    Rot rot = get_entity_rotation(ctx->jvm, ctx->player);
    Vec3 pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Vec3 prev = java_get_entity_prev_pos_x(ctx->jvm, ctx->player);
    Vec3 start = vec3_lerp(prev, pos, (double)h->partial);
    Vec3 pt = prev, vel = pos;
    throwable_velocity(ctx, &start, rot, h->partial, &pt, &vel);
    for (unsigned i = 0; i < (unsigned)trajectories_path_length; i++) {
        pt = vec3_add(pt, vel);
        trajectories_screen_points[i].x = (float)(pt.x - h->cam.x);
        trajectories_screen_points[i].y = (float)(pt.y - h->cam.y);
        trajectories_screen_points[i].z = (float)(pt.z - h->cam.z);
        vel = vec3_scale(vel, AIR_DRAG);
        vel.y -= trajectories_gravity;
    }
    (*ptr_to_pGlEnable)(0xbe2);   // GL_BLEND
    (*ptr_to_pGlLineWidth)(trajectories_line_width);
    if (trajectories_hit_colour_fourth)
        (*ptr_to_pGlColor3f)(trajectories_hit_colour, trajectories_hit_colour_second, trajectories_hit_colour_third);
    else
        (*ptr_to_pGlColor3f)(trajectories_miss_colour, trajectories_miss_colour_second, trajectories_miss_colour_third);
    (*ptr_to_pGlEnableClientState)(0x8074);                    // GL_VERTEX_ARRAY
    (*ptr_to_pGlVertexPointer)(3, 0x1406, 0, trajectories_screen_points);
    (*ptr_to_pGlDrawArrays)(3, 0, trajectories_path_length);         // GL_LINE_STRIP
    if (trajectories_something_hit == 1) {
        Vec3 mid = vec3_lerp(trajectories_hit_box.lo, trajectories_hit_box.hi, 0.5);
        Vec3 half = vec3_sub(trajectories_hit_box.hi, mid);
        Vec3 back = vec3_scale(trajectories_hit_entity_movement_this_tick, 1.0 - (double)h->partial);
        Vec3 at = vec3_sub(vec3_sub(mid, h->cam), back);
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)((float)at.x, (float)at.y, (float)at.z);
        (*ptr_to_pGlScalef)((float)half.x, (float)half.y, (float)half.z);
        gl_array_xyz_float();
        gl_array_color2();
        (*ptr_to_pGlPopMatrix)();
    }
    disable_vertex_array_state();
}
