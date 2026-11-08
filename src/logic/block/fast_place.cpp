// Fast Place - place blocks quickly while holding right click.
//
// Minecraft repeats right click only every 4 ticks. While you hold the use key, this
// module clicks at "max_cps" clicks per second instead (after "activation_delay" ticks).
// It skips clicks that would put a block inside yourself, except now and then at random.
//
// Callback:
//   fast_place_tick  every tick
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Box { Vec3 lo, hi; };
struct Range { float lo, hi; };

// ---- settings ----
extern "C" float fast_place_max_cps;              // clicks per second
extern "C" float fast_place_activation_delay;     // ticks to hold before clicking starts

// ---- module state ----
extern "C" float fast_place_click_timer;               // ticks until the next click
extern "C" unsigned char fast_place_use_key_held_last_tick;
extern "C" int fast_place_activation_delay_left;
extern "C" char fast_place_random_walk_1[];            // random walk state

// Bounds of the random range used to sometimes click anyway (0.02 .. 0.08).
extern "C" const float random_range_bound_0_02, random_range_bound_0_08_in_fast_place;

// ---- game / engine functions ----
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(int, java_get_key_binding_press_time, (void *jvm, void *key_binding));             // clicks waiting to be handled
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));      // = one right click
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, block_entity_id, (void *jvm, void *entity));                         // 1 if the item is a block
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *hit));       // 1 = a block
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(Vec3, java_get_adjacent_block_position, (void *jvm, void *hit));                   // where the new block would go
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Box, aabb_make, (Vec3 lo, Vec3 hi));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(unsigned char, aabb_intersects, (Box a, Box b));
FN(Range, rot_make_floats, (float lo, float hi));
FN(unsigned char, walk_should_change, (void *state, Range r, double d));   // random yes/no

enum { ITEM_LADDER = 65 };

// Would a click right now be useful?
static bool should_click(Ctx *ctx) {
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 1)
        return walk_should_change(fast_place_random_walk_1, rot_make_floats(random_range_bound_0_02, random_range_bound_0_08_in_fast_place), 20.0);

    // Ladders can always be placed.
    void *stack = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (stack && java_call_static_item_get_id_from_item(ctx->jvm, stack) == ITEM_LADDER)
        return true;

    // Only when the new block would not overlap the player (or now and then at random).
    Vec3 cell_pos = java_get_adjacent_block_position(ctx->jvm, hit);
    Box cell = aabb_make(cell_pos, vec3_add(cell_pos, vec3_make(1.0, 1.0, 1.0)));
    Box player_box = java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, ctx->player));
    return !aabb_intersects(cell, player_box) ||
           walk_should_change(fast_place_random_walk_1, rot_make_floats(random_range_bound_0_02, random_range_bound_0_08_in_fast_place), 20.0);
}

extern "C" void fast_place_tick(Hook *hook) {
    // Count the click timer down.
    float timer = fast_place_click_timer;
    if (timer > 0.0f) {
        timer += -1.0f;
        fast_place_click_timer = timer;
        if (timer > 0.0f)
            return;
    }

    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    void *use_key = java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
    unsigned char was_held = fast_place_use_key_held_last_tick;
    unsigned char held = java_get_key_binding_key_code(ctx->jvm, use_key);
    fast_place_use_key_held_last_tick = held;

    // Not holding right click: reset the activation delay.
    if (!held) {
        fast_place_activation_delay_left = (int)fast_place_activation_delay;
        return;
    }
    if (fast_place_activation_delay_left > 0) {
        fast_place_activation_delay_left -= 1;
        return;
    }
    // A click is already waiting.
    if (java_get_key_binding_press_time(ctx->jvm, use_key) > 0)
        return;

    timer = fast_place_click_timer;
    if (!(0.0f >= timer))
        return;

    // Click as many times as fit in this tick (more than one at a high cps).
    // The very first press clicks by itself, so that one is not repeated.
    bool first_press = !was_held;
    for (;;) {
        timer = (float)((double)timer + 20.0 / (double)fast_place_max_cps);   // 20 ticks per second
        fast_place_click_timer = timer;
        if (!first_press && block_entity_id(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player))) {
            if (should_click(ctx))
                java_increment_key_binding_press_time(ctx->jvm, use_key);
            timer = fast_place_click_timer;
        }
        first_press = false;
        if (!(0.0f >= timer))
            return;
    }
}
