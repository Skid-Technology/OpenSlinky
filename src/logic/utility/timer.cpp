// Timer - run the game faster (or slower).
//
// Every frame the game speed is set to "speed" while the conditions hold (mouse pressed,
// falling, just hurt), and back to normal (1.0) otherwise.
//
// Callbacks (descriptor slots +0x58 / +0x98):
//   timer_on_frame   every frame
//   timer_on_toggle  module switched off: normal speed
#include "slinky/engine.h"

// ---- settings ----
extern "C" float timer_speed_value;                  // "speed"
extern "C" unsigned char timer_require_mouse_value;  // "require_mouse"
extern "C" unsigned char timer_require_falling_value;   // "require_falling"
extern "C" unsigned char timer_require_damage_value;    // "require_damage": only while hurt

extern "C" char timer_module_descriptor[];

struct Vec3 { double x, y, z; };

struct TimerState {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *tag;           // +0x10 (toggle) module descriptor that was toggled
    bool flag;           // +0x18 (toggle) true = switched on
};

// ---- game / engine functions ----
FN(void *, get_timer, (void *jvm, void *timer));                                         // the game's tick timer
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                   // a mouse button is held
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(void, java_set_timer_timer_speed, (void *jvm, void *timer, float speed));

static bool conditions_met(Ctx *ctx) {
    if (timer_require_mouse_value == 1 && !vertex_pointer_current(ctx))
        return false;
    if (timer_require_falling_value == 1) {
        if (java_get_entity_on_ground(ctx->jvm, ctx->player))
            return false;
        if (java_get_entity_motion_x(ctx->jvm, ctx->player).y >= 0.0)
            return false;
    }
    if (timer_require_damage_value == 1) {
        if (ctx->in_game != 1)
            return false;
        if (java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) == 0)
            return false;
    }
    return true;
}

extern "C" void timer_on_frame(TimerState *s) {
    Ctx *ctx = s->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);
    float speed = conditions_met(ctx) ? timer_speed_value : 1.0f;
    java_set_timer_timer_speed(ctx->jvm, timer, speed);
}

extern "C" void timer_on_toggle(TimerState *s) {
    if (s->tag != (void *)timer_module_descriptor || s->flag != 0)
        return;
    Ctx *ctx = s->ctx;
    java_set_timer_timer_speed(ctx->jvm, get_timer(ctx->jvm, ctx->minecraft), 1.0f);
}
