// Bridge Assist - sneak automatically at block edges while bridging backwards.
//
// While you walk backwards placing blocks, this module holds sneak just before you would
// step off the edge, and lets go again a moment later. It can also restrict placing to
// clicks that hit the edge of the block below you.
//
// Callbacks:
//   bridge_assist_disable       module switched off: give the sneak key back
//   bridge_assist_on_late_tick  every tick: keep the module's keybind state in sync
//   bridge_assist_tick          every tick: hold / release sneak
//   bridge_assist_place         right click: decide if placing is allowed
//   bridge_assist_edge_check    helper: is the ground under our next position air?
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Range { float lo, hi; };
struct Box { double x0, y0, z0, x1, y1, z1; };

struct PlaceEvt {
    char padding_00[8];
    Ctx *ctx;                 // +0x08 game context
    unsigned char cancelled;  // +0x10 placing already cancelled
    unsigned char allow;      // +0x11 set to 1 to allow placing
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" int bridge_assist_activation_mode;                 // 0 = off, 1 = on, 2 = toggle with the keybind
extern "C" unsigned char bridge_assist_only_while_sneak_held; // only while you hold sneak yourself
extern "C" unsigned char bridge_assist_only_when_looking_down;    // only with pitch >= 65
extern "C" unsigned char bridge_assist_only_while_not_rising;     // not while walking forwards
extern "C" unsigned char bridge_assist_only_few_ticks_after_landing;   // only shortly after holding blocks
extern "C" unsigned char bridge_assist_also_air_while_falling;    // also in the air while not falling
extern "C" unsigned char bridge_assist_randomise;             // randomise margin and release delay
extern "C" unsigned char bridge_assist_edge_only_placing;     // only allow placing on the edge
extern "C" float bridge_assist_edge_margin;                   // how close to the edge before sneaking (blocks)
extern "C" float bridge_assist_release_delay;                 // ticks to keep sneaking after the edge is safe

// ---- module state ----
extern "C" char bridge_assist_module_descriptor[];
extern "C" unsigned char bridge_assist_sneak_held_by_us;      // we are controlling the sneak key
extern "C" unsigned char bridge_assist_placing;               // active this tick
extern "C" unsigned char bridge_assist_restrict_placing_edge; // placing is restricted to the edge this tick
extern "C" int bridge_assist_cooldown_after_sneaking_ticks;   // 20 after placing a block, counts down
extern "C" int bridge_assist_ticks_left_hold;                 // "only_few_ticks_after_landing" countdown
extern "C" float bridge_assist_release_timer;                 // ticks left before sneak is released
extern "C" char bridge_assist_rng_states[], bridge_assist_rng_states_second[], bridge_assist_rng_states_third[];   // random walk states

// Bounds of random ranges (the name says the value: neg_0_5 = -0.5, 0_04 = 0.04).
extern "C" const float random_range_bound_neg_0_5, random_range_bound_0_5_in_bridge_assist, random_range_bound_0_04_in_bridge_assist,
                       random_range_bound_0_08_in_bridge_assist, random_range_bound_3_in_bridge_assist, random_range_bound_0_15_in_bridge_assist;

// ---- game / engine functions ----
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_sneak_key, (void *jvm, void *game_settings));
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(void, track_owner, (void *ctx, void *mod, unsigned char toggle));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *hit));       // 1 = a block
FN(unsigned, java_get_variant, (void *jvm, void *hit));                               // side of the block that was hit (>= 2: a vertical side)
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *k));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, block_entity_id, (void *jvm, void *entity));                         // 1 if the item is a block
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(double, libc_fmod, (double x, double y));
FN(double, libc_floor, (double x));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(void *, java_get_moving_object_position_hit_vec, (void *jvm, void *hit));
FN(Vec3, java_get_vec3_x_coord, (void *jvm, void *bp));
FN(Vec3, java_get_adjacent_block_position, (void *jvm, void *hit));
FN(float, java_get_entity_living_base_move_forward, (void *jvm, void *entity_living_base));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(float, java_get_entity_living_base_land_movement_factor, (void *jvm, void *entity_living_base));
FN(Vec3, horizontal_motion, (Rot rotation, float fwd, float strafe, float speed));
FN(void *, java_call_variant, (void *jvm, void *world, int x, int y, int z));          // block at x, y, z
FN(int, get_block_id, (void *jvm, void *block));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double d));           // next value of a random walk inside the range
FN(unsigned char, walk_should_change, (void *state, Range r, double d));
FN(float, random_rotation, (Range r));                              // random value in the range

static double abs_d(double x) { return __builtin_fabs(x); }
static float abs_f(float x) { return __builtin_fabsf(x); }

// On disable: give the sneak key back to the player.
extern "C" void bridge_assist_disable(EvHook *hook) {
    if (hook->ev != bridge_assist_module_descriptor || hook->on || !(bridge_assist_sneak_held_by_us & 1))
        return;
    Ctx *ctx = hook->ctx;
    void *sneak_key = java_get_game_settings_sneak_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
    java_set_key_binding_pressed(ctx->jvm, sneak_key, java_get_key_binding_key_code(ctx->jvm, sneak_key));
    bridge_assist_sneak_held_by_us = 0;
}

extern "C" void bridge_assist_on_late_tick(Hook *hook) {
    if (bridge_assist_placing == 1)
        track_owner(hook->ctx, bridge_assist_module_descriptor, bridge_assist_activation_mode == 2);
}

// Where will the player be next tick, and is there no block under any corner of that spot
// (shrunk by the edge margin)? Returns 1 when we would walk off an edge.
extern "C" unsigned char bridge_assist_edge_check(void *jvm, void *game_settings, void *world, void *player) {
    // Middle of the feet, just below them.
    Box box = java_get_axis_aligned_bb_min_x(jvm, java_get_entity_bounding_box(jvm, player));
    Vec3 feet = vec3_make((box.x0 + box.x1) * 0.5, box.y0 + -0.01, (box.z0 + box.z1) * 0.5);
    Vec3 motion = java_get_entity_motion_x(jvm, player);

    double margin = bridge_assist_edge_margin;
    if (bridge_assist_randomise == 1) {
        margin -= walk_update(bridge_assist_rng_states, rot_make_floats(0.0f, random_range_bound_0_15_in_bridge_assist), 5.0);
        if (0.0 > margin)
            margin = 0.0;
    }

    // The movement keys held, as input values.
    void *forward_key = java_get_game_settings_forward_key(jvm, game_settings);
    void *back_key = java_get_game_settings_back_key(jvm, game_settings);
    void *left_key = java_get_game_settings_left_key(jvm, game_settings);
    void *right_key = java_get_game_settings_right_key(jvm, game_settings);
    float forward = is_key_down(jvm, forward_key) ? 1.0f : 0.0f;
    if (is_key_down(jvm, back_key))
        forward += -1.0f;
    float strafe = is_key_down(jvm, left_key) ? 1.0f : 0.0f;
    if (is_key_down(jvm, right_key))
        strafe += -1.0f;

    // Next position = feet + motion + what the keys add.
    Rot rotation = get_entity_rotation(jvm, player);
    Vec3 push = horizontal_motion(rotation, forward, strafe, java_get_entity_living_base_land_movement_factor(jvm, player));
    Vec3 next = vec3_add(feet, vec3_add(motion, push));

    // The (up to 4) blocks under the spot, shrunk by the margin. Any solid one = safe.
    int x0 = (int)libc_floor(next.x - margin);
    int z0 = (int)libc_floor(next.z - margin);
    int y = (int)libc_floor(next.y);
    double x1_floor = libc_floor(next.x + margin);
    double z1_floor = libc_floor(margin + next.z);
    if (get_block_id(jvm, java_call_variant(jvm, world, x0, y, z0)))
        return 0;
    int z1 = (int)z1_floor;
    if (z0 != z1 && get_block_id(jvm, java_call_variant(jvm, world, x0, y, z1)))
        return 0;
    int x1 = (int)x1_floor;
    if (x0 == x1)
        return 1;
    if (get_block_id(jvm, java_call_variant(jvm, world, x1, y, z0)))
        return 0;
    if (z0 == z1)
        return 1;
    return !get_block_id(jvm, java_call_variant(jvm, world, x1, y, z1));
}

extern "C" void bridge_assist_tick(Hook *hook) {
    bridge_assist_restrict_placing_edge = 0;
    bridge_assist_placing = 0;
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    void *game_settings = get_game_settings(ctx->jvm, ctx->minecraft);
    if (bridge_assist_cooldown_after_sneaking_ticks > 0)
        bridge_assist_cooldown_after_sneaking_ticks -= 1;
    void *sneak_key = java_get_game_settings_sneak_key(ctx->jvm, game_settings);
    unsigned char sneak_held = java_get_key_binding_key_code(ctx->jvm, sneak_key);

    // Conditions. If one fails, give the sneak key back.
    bool conditions_ok = true;
    if (bridge_assist_only_while_sneak_held != sneak_held)
        conditions_ok = false;
    else if (bridge_assist_only_when_looking_down == 1 && 65.0f > get_entity_rotation(ctx->jvm, ctx->player).pitch)
        conditions_ok = false;
    else if (bridge_assist_only_while_not_rising == 1 && java_get_entity_living_base_move_forward(ctx->jvm, ctx->player) > 0.0f)
        conditions_ok = false;
    if (conditions_ok) {
        bridge_assist_placing = bridge_assist_activation_mode != 0;
        if (bridge_assist_only_few_ticks_after_landing == 1) {
            // Holding blocks keeps the countdown at 2; otherwise it runs out.
            int ticks_left;
            if (block_entity_id(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player))) {
                ticks_left = 2;
            } else {
                ticks_left = bridge_assist_ticks_left_hold;
                if (ticks_left <= 0)
                    conditions_ok = false;
                ticks_left -= 1;
            }
            if (conditions_ok)
                bridge_assist_ticks_left_hold = ticks_left;
        }
    }
    if (!conditions_ok) {
        if (bridge_assist_sneak_held_by_us == 1)
            java_set_key_binding_pressed(ctx->jvm, sneak_key, sneak_held);
        bridge_assist_sneak_held_by_us = 0;
        return;
    }
    bridge_assist_sneak_held_by_us = 1;

    // What to do with the sneak key this tick.
    enum { KEEP, CHECK_EDGE, COUNT_DOWN } action;
    if (is_key_down(ctx->jvm, sneak_key)) {
        // Sneaking now: release once the edge is safe (after the release delay).
        if (bridge_assist_only_while_sneak_held == 1) {
            if (java_get_entity_living_base_move_forward(ctx->jvm, ctx->player) >= 0.0f || bridge_assist_cooldown_after_sneaking_ticks <= 0)
                action = KEEP;
            else
                action = CHECK_EDGE;
        } else if (java_get_entity_on_ground(ctx->jvm, ctx->player)) {
            action = CHECK_EDGE;
        } else if (bridge_assist_also_air_while_falling == 1 && !(0.0 > java_get_entity_motion_x(ctx->jvm, ctx->player).y)) {
            action = CHECK_EDGE;
        } else {
            action = COUNT_DOWN;
        }
    } else {
        // Not sneaking: press sneak if we are about to walk off the edge.
        if (!java_get_entity_on_ground(ctx->jvm, ctx->player)) {
            if (bridge_assist_also_air_while_falling != 1 || 0.0 > java_get_entity_motion_x(ctx->jvm, ctx->player).y)
                return;
        }
        if (!bridge_assist_edge_check(ctx->jvm, game_settings, ctx->world, ctx->player))
            return;
        java_set_key_binding_pressed(ctx->jvm, sneak_key, 1);
        action = KEEP;
    }

    // Still at the edge: keep sneaking.
    if (action == CHECK_EDGE && bridge_assist_edge_check(ctx->jvm, game_settings, ctx->world, ctx->player))
        action = KEEP;

    if (action != KEEP) {
        // Count the release timer down; at 0 release sneak and start a new (random) delay.
        bridge_assist_release_timer += -1.0f;
        if (!(bridge_assist_release_timer > 0.0f)) {
            java_set_key_binding_pressed(ctx->jvm, sneak_key, 0);
            float delay = bridge_assist_release_delay;
            if (bridge_assist_randomise == 1) {
                delay += walk_update(bridge_assist_rng_states_second, rot_make_floats(random_range_bound_neg_0_5, random_range_bound_0_5_in_bridge_assist), 30.0);
                if (walk_should_change(bridge_assist_rng_states_third, rot_make_floats(random_range_bound_0_04_in_bridge_assist, random_range_bound_0_08_in_bridge_assist), 60.0))
                    delay += random_rotation(rot_make_floats(0.0f, random_range_bound_3_in_bridge_assist));
            }
            bridge_assist_release_timer = delay + bridge_assist_release_timer;
            return;
        }
    }
    bridge_assist_restrict_placing_edge = bridge_assist_edge_only_placing;
}

// Is the crosshair on the outer edge of the side of the block we are standing on,
// while walking straight backwards at a diagonal angle?
static bool aimed_at_edge_below(Ctx *ctx) {
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 1 || java_get_variant(ctx->jvm, hit) < 2 ||
        !java_get_entity_on_ground(ctx->jvm, ctx->player))
        return false;

    // Only back held (no left/right).
    void *game_settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *back_key = java_get_game_settings_back_key(ctx->jvm, game_settings);
    void *left_key = java_get_game_settings_left_key(ctx->jvm, game_settings);
    void *right_key = java_get_game_settings_right_key(ctx->jvm, game_settings);
    if (!is_key_down(ctx->jvm, back_key) || is_key_down(ctx->jvm, left_key) || is_key_down(ctx->jvm, right_key))
        return false;

    // Facing diagonally (30..60 degrees within each quarter turn).
    float yaw = abs_f((float)libc_fmod(get_entity_rotation(ctx->jvm, ctx->player).yaw, 90.0));
    if (30.0f > yaw || yaw > 60.0f)
        return false;

    // The point hit must be within 1 block sideways and up to 1 block below the feet.
    Vec3 pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (java_variant(ctx->jvm))
        pos = vec3_add_eye_height_sneak(pos);
    Vec3 hit_pos = java_get_vec3_x_coord(ctx->jvm, java_get_moving_object_position_hit_vec(ctx->jvm, hit));
    double dy = hit_pos.y - pos.y;
    if (abs_d(hit_pos.x - pos.x) > 1.0 || -1.0 > dy || dy > 0.0 || abs_d(hit_pos.z - pos.z) > 1.0)
        return false;

    // The point hit must not be near a corner of the face. Standing right on the new block
    // the band is wider (0.25) than otherwise (0.15).
    Vec3 new_block = java_get_adjacent_block_position(ctx->jvm, hit);
    bool inside_z = new_block.z < pos.z && pos.z < new_block.z + 1.0;
    bool inside_x = new_block.x < pos.x && pos.x < new_block.x + 1.0;
    double frac_x = abs_d(libc_fmod(hit_pos.x, 1.0));
    double frac_z = libc_fmod(hit_pos.z, 1.0);
    double edge = inside_x && inside_z ? 0.25 : 0.15;
    double far = 1.0 - edge;
    if (!(far < frac_x) && !(frac_x < edge))
        return true;
    frac_z = abs_d(frac_z);
    return !(far < frac_z) && !(frac_z < edge);
}

// Right click: with "edge_only_placing", only allow clicks on the edge below us.
extern "C" void bridge_assist_place(PlaceEvt *h) {
    if (h->cancelled || bridge_assist_cooldown_after_sneaking_ticks == 20)
        return;
    Ctx *ctx = h->ctx;
    if (bridge_assist_restrict_placing_edge == 1 && aimed_at_edge_below(ctx)) {
        h->allow = 1;
        return;
    }
    if (block_entity_id(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player)))
        bridge_assist_cooldown_after_sneaking_ticks = 20;
}
