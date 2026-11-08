// 0x2f000: health-bar style line drawing (immediate-mode GL), comparator, hurt-time hook
#include "slinky/engine.h"
typedef void (*GlColor3f)(float, float, float);
typedef void (*GlBegin)(int);
typedef void (*GlVertex2f)(float, float);
typedef void (*GlEnd)();
extern "C" GlColor3f *ptr_to_pGlColor3f;
extern "C" GlBegin *ptr_to_pGlBegin;
extern "C" GlVertex2f *ptr_to_pGlVertex2f;
extern "C" GlEnd *ptr_to_pGlEnd;
extern "C" float one_in_healthbar_draw, healthbar_green_level;
FN(float, lerp_unclamped_float, (float a, float b, float t));   // lerp

struct Bar { char padding_00[0x44]; float fill_fraction, overlay_fraction; };

extern "C" void draw_health_bar(Bar *b, float x, float y0, float y1) {
    float m = lerp_unclamped_float(y1, y0, b->fill_fraction);
    GlColor3f *color = ptr_to_pGlColor3f;
    (*color)(0.0f, 0.0f, 0.0f);
    GlBegin *begin = ptr_to_pGlBegin;
    (*begin)(1);
    GlVertex2f *vertex = ptr_to_pGlVertex2f;
    (*vertex)(x, y0);
    (*vertex)(x, m);
    GlEnd *end = ptr_to_pGlEnd;
    (*end)();
    if (b->overlay_fraction > 0.0f) {
        float m2 = lerp_unclamped_float(m, y1, b->overlay_fraction);
        float c0 = one_in_healthbar_draw;

        (*color)(c0, healthbar_green_level, 0.0f);
        (*begin)(1);
        (*vertex)(x, m);
        (*vertex)(x, m2);
        (*end)();
        m = m2;
    }
    GlColor3f cf = *color;
    float fill_fraction = b->fill_fraction;
    float c = one_in_healthbar_draw;
    float r = lerp_unclamped_float(c, 0.0f, fill_fraction);
    float g = lerp_unclamped_float(0.0f, c, b->fill_fraction);
    cf(r, g, 0.0f);
    (*begin)(1);
    (*vertex)(x, m);
    (*vertex)(x, y1);
    (*end)();
}

extern "C" int compare_healthbar_height(char *left, char *right) { return (int)(*(float *)(right + 0x3c) - *(float *)(left + 0x3c)); }

extern "C" int reach_last_value_seen_from_java, reach_cooldown_ticks;
extern "C" unsigned char reach_require_sprint;
extern "C" float reach_activate_time, reach_distance;
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));
struct Ev { char padding_00[8]; Ctx *ctx; unsigned char on; char padding_11[7]; double reach; };
extern "C" void healthbar_render(Ev *event) {
    int n = reach_cooldown_ticks;
    if (n > 0) reach_cooldown_ticks = --n;
    Ctx *ctx = event->ctx;
    if (reach_require_sprint == 1) {
        if (!java_call_entity_is_sprinting(ctx->jvm, ctx->player)) return;
        n = reach_cooldown_ticks;
    }
    if (n == 0) {
        int t = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
        if (t == reach_last_value_seen_from_java) {
            n = reach_cooldown_ticks;
        } else {
            reach_last_value_seen_from_java = t;
            reach_cooldown_ticks = 9;
            n = 9;
        }

    }
    SHARED_EPILOG();  // original keeps one shared tail for both arms
    if (reach_activate_time > (float)n) {
        event->on = 1;
        event->reach = reach_distance;
    }
}
