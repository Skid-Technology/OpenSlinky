// Player ESP - draw boxes around players through walls, with health bars.
//
// Every tick the module collects the players within "range" (at most 100; bots, and with
// "hide_friendlies" friends/teammates, are skipped), with their colour (team colour, or
// friend / other colour; the combat target in its own colour) and health.
// Every frame it draws for each a 3D box ("mode" 1) or a 2D rectangle around the projected box
// ("mode" 0), turning red for a moment when they get hurt ("hurt_effect"), plus a health bar.
//
// Callbacks:
//   player_esp_tick           every tick: collect the players
//   player_esp_render_mode_1  every frame (world): 3D boxes
//   player_esp_render_mode_0  every frame (screen): 2D boxes
#include "slinky/engine.h"
#include "slinky/jids.h"


struct Vec3 { double x, y, z; };
struct Color3 { float r, g, b; };
struct Pair { float x, y; };
struct Player {                     // 0x50 bytes at 0x2f7e20
    Vec3 prev, pos;
    Color3 color;
    float dist2;
    int hurt;                       // hurt ticks left
    float health;                   // (health + absorption) / (max + absorption)
    float absorb;                   // absorption / health
    unsigned char target;           // current combat target
};
struct TickEvt { char padding_00[8]; Ctx *ctx; };
struct Render3d { char padding_00[8]; Vec3 cam; Ctx *ctx; float partial; };
struct Render2d { char padding_00[8]; Vec3 cam; char padding_20[8]; float width, height; char padding_30[0x10]; float partial; };

extern "C" unsigned long long player_esp_target_count;     // count
extern "C" Player player_esp_targets[100];
extern "C" float player_esp_line_width;                  // line width
extern "C" int player_esp_mode;                    // mode: 0 2D, 1 3D
extern "C" unsigned char player_esp_show_2d_box;          // draw boxes for every player (else only the target)
extern "C" unsigned char player_esp_hurt_effect;          // hurt effect
extern "C" unsigned char player_esp_filled;          // filled
extern "C" unsigned char player_esp_health_bar;          // health bar
extern "C" float player_esp_range;                  // range
extern "C" Color3 player_esp_target_colour;                 // target colour
extern "C" Color3 player_esp_friend_other_colour, player_esp_friend_other_colour_second;    // friend / other colour
extern "C" unsigned char player_esp_hide_friendlies;          // also skip invisible
extern "C" unsigned char player_esp_override_target_color;          // highlight target
extern "C" int player_esp_color_mode;                    // colour mode: 0 team, else friend / other
static const float BOX_HALF_WIDTH = 0.4f;     // the unit box (-1..1) scaled to 0.8 x 2 blocks
static const float HURT_FADE_TICKS = 10.0f;   // hurt time (ticks) that is fully red...
static const float HURT_FADE_SCALE = 0.1f;    // ...times 0.1 = 1
extern "C" const Vec3 player_esp_box_corners_around_feet[8];         // box corners around the feet
extern "C" void (**ptr_to_pGlPushMatrix)();                                 // glPushMatrix
extern "C" void (**ptr_to_pGlPopMatrix)();                                 // glPopMatrix
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);              // glTranslatef
extern "C" void (**ptr_to_pGlScalef)(float, float, float);              // glScalef
extern "C" void (**ptr_to_pGlRotatef)(float, float, float, float);       // glRotatef
extern "C" void (**ptr_to_pGlLineWidth)(float);                            // glLineWidth
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);              // glColor3f
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);       // glColor4f
extern "C" void (**ptr_to_pGlEnable)(unsigned);                         // glEnable

FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void *, java_get_minecraft_render_manager, (void *jvm, void *minecraft));
FN(float, java_get_render_manager_player_view_y, (void *jvm, void *minecraft));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(int, entity_flags_get, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_is_dead, (void *jvm, void *entity));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(void *, get_refill_state, ());
FN(int, entity_name_colour, (void *jvm, void *entity));
FN(Color3, minecraft_color_code_to_rgb, (int team));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(float, java_call_entity_player_get_absorption_amount, (void *jvm, void *entity_player));
FN(float, java_call_entity_living_base_get_max_health, (void *jvm, void *entity_living_base));
FN(float, limit_rotation_delta, (float lo, float hi, float v));
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, compare_healthbar_height, (const void *left, const void *right));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(unsigned char, project_point, (void *view, Vec3 p, Pair *out));
FN(void, draw_health_bar, (Player *p, float x, float y0, float y1));
FN(void, gl_array_xyz_float, ());
FN(void, gl_array_color1, ());
FN(void, gl_array_color2, ());
FN(void, gl_array_xy_float, ());
FN(void, gl_array_color3, ());
FN(void, gl_array_color4, ());
FN(void, disable_vertex_array_state, ());

typedef unsigned char (*IsSameObject)(void *env, void *a, void *b);

// Fade the colour towards red while the player is hurt (hurt time counts 10 -> 0).
static void hurt_tint(Player *p, float partial, float cap, float scale, float one,
                                                            float &r, float &g, float &b) {
    if (p->hurt <= 0 || !player_esp_hurt_effect)
        return;
    float x = (float)p->hurt - partial;
    float k = (cap < x ? cap : x) * scale;
    r = lerp_unclamped_float(r, one, k);
    g = lerp_unclamped_float(g, 0.0f, k);
    b = lerp_unclamped_float(b, 0.0f, k);
}

extern "C" void player_esp_tick(TickEvt *h) {
    Ctx *ctx = h->ctx;
    player_esp_target_count = 0;
    if (!ctx->world)
        return;
    Vec3 position = java_get_entity_pos_x(ctx->jvm, ctx->player);
    void *list = java_get_world_player_entities(ctx->jvm, ctx->world);
    int skip = player_esp_hide_friendlies * 5 + 2;
    for (int i = 0; i < java_call_list_size(ctx->jvm, list); i++) {
        void *element = java_call_list_get(ctx->jvm, list, i);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        if ((*env)->IsSameObject(env, ctx->player, element))
            continue;
        int flags = entity_flags_get(ctx->jvm, element);
        if (flags & skip)
            continue;
        if (java_get_entity_is_dead(ctx->jvm, element))
            continue;
        Vec3 pos = java_get_entity_pos_x(ctx->jvm, element);
        double d2 = vec3_distance_sq(pos, position);
        if (d2 > (double)(player_esp_range * player_esp_range))
            continue;
        Color3 col;
        unsigned char target = 0;
        if (player_esp_override_target_color == 1 && (*env)->IsSameObject(env, element, get_refill_state())) {
            col = player_esp_target_colour;
            target = 1;
        } else if (player_esp_color_mode) {
            col = (flags & 5) ? player_esp_friend_other_colour : player_esp_friend_other_colour_second;
        } else {
            col = minecraft_color_code_to_rgb(entity_name_colour(ctx->jvm, element));
        }
        Player *p = &player_esp_targets[player_esp_target_count++];
        p->pos = pos;
        p->prev = java_get_entity_prev_pos_x(ctx->jvm, element);
        p->color = col;
        p->dist2 = (float)d2;
        p->hurt = java_get_entity_living_base_hurt_time(ctx->jvm, element);
        p->target = target;
        float health = java_call_entity_living_base_get_health(ctx->jvm, element);
        float abs = java_call_entity_player_get_absorption_amount(ctx->jvm, element);
        abs = 0.0f > abs ? 0.0f : abs;
        float total = java_call_entity_living_base_get_max_health(ctx->jvm, element) + abs;
        p->health = total != 0.0f ? limit_rotation_delta(0.0f, 1.0f, (health + abs) / total) : 0.0f;
        p->absorb = health != 0.0f ? limit_rotation_delta(0.0f, 1.0f, abs / health) : 0.0f;
        if (player_esp_target_count == 100)
            break;
    }
    libc_qsort(player_esp_targets, player_esp_target_count, sizeof(Player), compare_healthbar_height);
}

extern "C" void player_esp_render_mode_1(Render3d *h) {
    if (!player_esp_target_count || player_esp_mode != 1)
        return;
    (*ptr_to_pGlEnable)(0xbe2);   // GL_BLEND
    (*ptr_to_pGlLineWidth)(player_esp_line_width);
    float partial = h->partial;
    Ctx *ctx = h->ctx;
    float yaw = java_get_render_manager_player_view_y(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    yaw = -yaw;
        for (unsigned long long i = 0; i < player_esp_target_count; i++) {
        Player *p = &player_esp_targets[i];
        Vec3 r = vec3_sub(vec3_lerp(p->prev, p->pos, partial), h->cam);
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)((float)r.x, (float)(r.y + 1.0), (float)r.z);
        (*ptr_to_pGlScalef)(BOX_HALF_WIDTH, 1.0f, BOX_HALF_WIDTH);
        if (player_esp_show_2d_box || p->target == 1) {
            gl_array_xyz_float();
            float cr = p->color.r, cg = p->color.g, cb = p->color.b;
            hurt_tint(p, partial, HURT_FADE_TICKS, HURT_FADE_SCALE, 1.0f, cr, cg, cb);
            if (player_esp_filled == 1) {
                (*ptr_to_pGlColor4f)(cr, cg, cb, 0.2f);
                gl_array_color1();
            }
            (*ptr_to_pGlColor3f)(cr, cg, cb);
            gl_array_color2();
            disable_vertex_array_state();
        }
        if (player_esp_health_bar == 1) {
            (*ptr_to_pGlRotatef)(yaw, 0.0f, 1.0f, 0.0f);
            (*ptr_to_pGlColor4f)(0.0f, 1.0f, 0.0f, 0.75f);
            draw_health_bar(p, 2.0f, 1.0f, -1.0f);
        }
        (*ptr_to_pGlPopMatrix)();
    }
}

extern "C" void player_esp_render_mode_0(Render2d *h) {
    if (!player_esp_target_count || player_esp_mode != 0)
        return;
    (*ptr_to_pGlEnable)(0xbe2);   // GL_BLEND
    (*ptr_to_pGlLineWidth)(player_esp_line_width);
    float partial = h->partial;
    void *view = &h->cam;
    for (unsigned long long i = 0; i < player_esp_target_count; i++) {
        Player *p = &player_esp_targets[i];
        Vec3 r = vec3_sub(vec3_lerp(p->prev, p->pos, partial), h->cam);
        Pair s;
        if (!project_point(view, vec3_add(r, player_esp_box_corners_around_feet[0]), &s))
            continue;
        float x0 = s.x, x1 = s.x, y0 = s.y, y1 = s.y;
        bool ok = true;
        for (int k = 1; k < 8; k++) {
            Pair q;
            if (!project_point(view, vec3_add(r, player_esp_box_corners_around_feet[k]), &q)) {
                ok = false;
                break;
            }
            if (x0 > q.x)
                x0 = q.x;
            else if (q.x > x1)
                x1 = q.x;
            if (y0 > q.y)
                y0 = q.y;
            else if (q.y > y1)
                y1 = q.y;
        }
        if (!ok)
            continue;
        bool min_on = !(0.0f > x0) && !(0.0f > y0) && !(x0 > h->width) && !(y0 > h->height);
        bool max_on = !(0.0f > x1) && !(x1 > h->width) && !(0.0f > y1) && !(y1 > h->height);
        if (!min_on && !max_on)
            continue;
        float cr = p->color.r, cg = p->color.g, cb = p->color.b;
        hurt_tint(p, partial, HURT_FADE_TICKS, HURT_FADE_SCALE, 1.0f, cr, cg, cb);
        if (player_esp_show_2d_box || p->target == 1) {
            gl_array_xy_float();
            (*ptr_to_pGlPushMatrix)();
            (*ptr_to_pGlTranslatef)(x0, y0, 0.0f);
            (*ptr_to_pGlScalef)(x1 - x0, y1 - y0, 1.0f);
            if (player_esp_filled == 1) {
                (*ptr_to_pGlColor4f)(cr, cg, cb, 0.25f);
                gl_array_color3();
            }
            (*ptr_to_pGlColor3f)(cr, cg, cb);
            gl_array_color4();
            (*ptr_to_pGlPopMatrix)();
            disable_vertex_array_state();
        }
        if (player_esp_health_bar == 1)
            draw_health_bar(p, x0 - player_esp_line_width + -1.0f + -4.0f, y0, y1);
    }
}
