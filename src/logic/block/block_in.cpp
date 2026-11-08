// Block In - build a box of blocks around yourself (roof first, then the walls).
//
// Every tick the module looks for the next free spot around the player, aims at it with
// smooth, slightly random head movement, and presses "use item" once the aim is on it.
// When nothing is left to place and you stand in the middle of your block, it turns itself off.
//
// Callbacks:
//   block_in_on_tick                        (unused, empty)
//   block_in_rotation_override_placement    the rotation sent to the server when placing
//   block_in_tick_while_placing             every tick: walk to the middle of the block / stand still
//   block_in_enable_remember_facing         module switched on/off
//   block_in_tick                           every tick: pick the next spot, aim, place
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Box { Vec3 lo, hi; };
struct Range { float lo, hi; };
struct Pair { float x, y; };                    // yaw, pitch difference
struct Offset3 { signed char dx, dy, dz; };
struct Offset2 { signed char dx, dz; };
struct Rot { float yaw, pitch; };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct RotHook {
    char padding_00[8];
    unsigned char override_yaw, override_pitch;   // +0x08 / +0x09 set to 1 to replace the rotation
    char padding_0a[2];
    Rot rot;                                       // +0x0c rotation to send
};

// ---- settings ----
extern "C" unsigned char block_in_require_ground;   // only while standing on the ground
extern "C" float block_in_aim_speed;                // how fast to turn the head

// ---- module state ----
extern "C" char block_in_module_descriptor[];
extern "C" unsigned char block_in_active;           // 1 while aiming/placing this tick
// 0 = not placing, 1 = walk to the middle of the block first, 2 = in the middle, stand still
extern "C" int block_in_rotation_mode;
extern "C" Rot block_in_placement_rotation;         // where to look while walking to the middle
// Spots already placed (a 64-bit mask): bits 0..11 the walls (3 heights x 4 sides), bits 32..41 the roof.
extern "C" void *block_in_placed_count;
extern "C" int block_in_facing_quadrant;            // which side to start the walls on (0..3, from the yaw)
extern "C" double block_in_last_y;                  // player height last tick (moving up or down resets the mask)
// States of the random walks that make the aim look human.
extern "C" char block_in_rng_states[], block_in_rng_states_second[], block_in_rng_states_third[], block_in_rng_states_fourth[], block_in_rng_states_fifth[], block_in_rng_states_in_block_in_d[];

// The 10 roof spots, relative to the block below the feet, and the 4 wall directions.
extern "C" const Offset3 block_in_roof_spots_relative_block_below[10];
extern "C" const Offset2 block_in_side_directions[4];

// Bounds of the random ranges used for aim points (the name says the value: 3_6 = 3.6, neg_0_25 = -0.25).
extern "C" const float random_range_bound_neg_0_25, random_range_bound_neg_0_4, random_range_bound_0_25_in_block_in, random_range_bound_0_4_in_block_in,
                       random_range_bound_0_6_in_block_in, random_range_bound_neg_0_1_in_block_in, random_range_bound_neg_0_2, random_range_bound_3_6,
                       random_range_bound_3_8, random_range_bound_1_1_in_block_in, random_range_bound_1_2_in_block_in, random_range_bound_0_8_in_block_in;

// ---- game / engine functions ----
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(float, libc_roundf, (float v));
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(double, libc_floor, (double x));
FN(double, libc_fmod, (double x, double y));
FN(void *, java_block_pos_init, (void *jvm, void *object, int y, int z));
FN(void *, java_call_world_get_block_state, (void *jvm, void *world, void *pos, int x, int y, int z));
FN(int, get_block_id, (void *jvm, void *block));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Box, aabb_make, (Vec3 lo, Vec3 hi));
FN(unsigned char, aabb_intersects, (Box a, Box b));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double d));        // next value of a random walk inside the range
FN(unsigned char, update_rotation_priority_peak, (int ms));      // may this module control the rotation?
FN(void, switch_best_hotbar_slot, (Ctx *ctx, void *mod, unsigned char on, void (*cb)(void *)));
FN(void, key_category, (void *));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, block_entity_id, (void *jvm, void *entity));   // 1 if the item is a block
FN(Rot, get_silent_rotation_offset, ());
FN(Rot, rot_add, (Rot a, Rot b));
FN(void *, java_call_entity_living_base_get_position, (void *jvm, void *entity_living_base));   // eye position
FN(Vec3, java_get_vec3_x_coord, (void *jvm, void *bp));
FN(Rot, rot_look_at, (Vec3 from, Vec3 to));
FN(Pair, rot_sub, (Rot a, Rot b));
FN(float, wrap_angle, (float deg));                               // to -180..180
FN(Pair, rot_scale, (Pair p, float k));
FN(float, limit_rotation_delta, (float lo, float hi, float v));  // clamp
FN(Pair, rot_lerp, (Pair a, Pair b, float k));
FN(void, apply_silent_rotation, (Pair r));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));   // = one right click
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));

static unsigned long long &placed_mask() { return *(unsigned long long *)&block_in_placed_count; }

extern "C" void block_in_on_tick(void *) {
}

// When placing: send our aim to the server, and in "walk to the middle" mode the walking direction.
extern "C" void block_in_rotation_override_placement(RotHook *hook) {
    if (block_in_active != 1)
        return;
    hook->override_yaw = 1;
    hook->override_pitch = 1;
    if (block_in_rotation_mode == 1)
        hook->rot = block_in_placement_rotation;
}

// While placing: hold forward to walk to the middle of the block (mode 1), never walk otherwise.
extern "C" void block_in_tick_while_placing(Hook *hook) {
    if (block_in_rotation_mode == 0)
        return;
    Ctx *ctx = hook->ctx;
    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *forward_key = java_get_game_settings_forward_key(ctx->jvm, settings);
    void *back_key = java_get_game_settings_back_key(ctx->jvm, settings);
    void *left_key = java_get_game_settings_left_key(ctx->jvm, settings);
    void *right_key = java_get_game_settings_right_key(ctx->jvm, settings);
    java_set_key_binding_pressed(ctx->jvm, forward_key, block_in_rotation_mode == 1);
    java_set_key_binding_pressed(ctx->jvm, back_key, 0);
    java_set_key_binding_pressed(ctx->jvm, left_key, 0);
    java_set_key_binding_pressed(ctx->jvm, right_key, 0);
}

// On enable: start fresh and remember which way we face. On disable: give the movement keys back.
extern "C" void block_in_enable_remember_facing(EvHook *hook) {
    if (hook->ev != block_in_module_descriptor)
        return;
    Ctx *ctx = hook->ctx;
    if (hook->on == 1) {
        block_in_placed_count = nullptr;
        int quadrant = (int)libc_roundf(get_entity_rotation(ctx->jvm, ctx->player).yaw / 90.0f);
        block_in_facing_quadrant = quadrant % 4;
        return;
    }
    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *forward_key = java_get_game_settings_forward_key(ctx->jvm, settings);
    void *back_key = java_get_game_settings_back_key(ctx->jvm, settings);
    void *left_key = java_get_game_settings_left_key(ctx->jvm, settings);
    void *right_key = java_get_game_settings_right_key(ctx->jvm, settings);
    java_set_key_binding_pressed(ctx->jvm, forward_key, java_get_key_binding_key_code(ctx->jvm, forward_key));
    java_set_key_binding_pressed(ctx->jvm, back_key, java_get_key_binding_key_code(ctx->jvm, back_key));
    java_set_key_binding_pressed(ctx->jvm, left_key, java_get_key_binding_key_code(ctx->jvm, left_key));
    java_set_key_binding_pressed(ctx->jvm, right_key, java_get_key_binding_key_code(ctx->jvm, right_key));
}

// Can a block be placed here? True for air, water and tall grass.
static bool is_open(Ctx *ctx, void *pos, int x, int y, int z) {
    unsigned block_id = get_block_id(ctx->jvm, java_call_world_get_block_state(ctx->jvm, ctx->world, pos, x, y, z));
    return block_id <= 31 && (0x80000301u >> block_id & 1);   // ids 0 (air), 8, 9 (water), 31 (tall grass)
}

static double abs_d(double x) { return __builtin_fabs(x); }
static float abs_f(float x) { return __builtin_fabsf(x); }

// Is the position in the middle part of its block (0.31 .. 0.69 on both x and z)?
static bool in_block_middle(double frac_x, double frac_z) {
    return 0.69 >= frac_x && frac_x >= 0.31 && frac_z >= 0.31 && 0.69 >= frac_z;
}

// Roof: look for the next roof spot. Returns false when the walls should be done first
// (or the roof is complete). bx/by/bz is the block below the feet.
static bool find_roof_spot(Ctx *ctx, void *pos, int bx, int by, int bz, Vec3 *target, unsigned char *bit) {
    unsigned long long placed = placed_mask();
    for (int i = 0; i < 10; i++) {
        const Offset3 &spot = block_in_roof_spots_relative_block_below[i];
        // Skip spots that are still open (and not placed by us yet).
        if (!(placed >> 32 >> i & 1) && is_open(ctx, pos, bx + spot.dx, by + spot.dy, bz + spot.dz))
            continue;
        if (i == 0)
            return false;   // already standing under a roof: do the walls
        *bit = (unsigned char)(32 + i);

        // Aim at a random point on the side of the block that is there.
        float fx = (float)bx, fy = (float)by, fz = (float)bz;
        Range height = rot_make_floats(random_range_bound_3_6, random_range_bound_3_8);
        switch (i == 9 ? 4 : (i - 1) & 3) {
        case 0:
            target->x = bx;
            target->y = fy + walk_update(block_in_rng_states, height, 0.5);
            target->z = fz + walk_update(block_in_rng_states_second, rot_make_floats(random_range_bound_neg_0_1_in_block_in, random_range_bound_neg_0_2), 0.5);
            break;
        case 1:
            target->x = fx + walk_update(block_in_rng_states, rot_make_floats(random_range_bound_1_1_in_block_in, random_range_bound_1_2_in_block_in), 0.5);
            target->y = fy + walk_update(block_in_rng_states_second, height, 0.5);
            target->z = bz;
            break;
        case 2:
            target->x = bx + 1;
            target->y = fy + walk_update(block_in_rng_states, height, 0.5);
            target->z = fz + walk_update(block_in_rng_states_second, rot_make_floats(random_range_bound_1_1_in_block_in, random_range_bound_1_2_in_block_in), 0.5);
            break;
        case 3:
            target->x = fx + walk_update(block_in_rng_states, rot_make_floats(random_range_bound_neg_0_1_in_block_in, random_range_bound_neg_0_2), 0.5);
            target->y = fy + walk_update(block_in_rng_states_second, height, 0.5);
            target->z = bz + 1;
            break;
        default:   // the last spot: aim at the underside, straight above
            target->x = fx + walk_update(block_in_rng_states, rot_make_floats(random_range_bound_0_4_in_block_in, random_range_bound_0_6_in_block_in), 0.5);
            target->y = by + 4;
            target->z = fz + walk_update(block_in_rng_states_second, rot_make_floats(random_range_bound_0_4_in_block_in, random_range_bound_0_6_in_block_in), 0.5);
            break;
        }
        return true;
    }
    return false;
}

// Walls: one column on each of the 4 sides, filled from the bottom (feet, head, above head).
static bool find_wall_spot(Ctx *ctx, void *pos, int bx, int by, int bz, Vec3 *target, unsigned char *bit) {
    unsigned long long placed = placed_mask();
    for (int d = 0; d < 4; d++) {
        int dir = (block_in_facing_quadrant + d) & 3;
        int dx = block_in_side_directions[dir].dx, dz = block_in_side_directions[dir].dz;
        int nx = bx + dx, nz = bz + dz;

        // Skip a side the player is standing in.
        Box player_box = java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, ctx->player));
        Vec3 lo = {(double)nx, (double)by, (double)nz};
        Box column = aabb_make(lo, vec3_add(lo, Vec3{1.0, 3.0, 1.0}));
        if (aabb_intersects(player_box, column))
            continue;

        // The lowest open spot that has a block under it to place against.
        int base = dir * 3;
        int level;
        Range r;
        if (!(placed >> base & 1) && is_open(ctx, pos, nx, by, nz) && !is_open(ctx, pos, nx, by - 1, nz)) {
            level = 0;
            r = rot_make_floats(random_range_bound_neg_0_25, random_range_bound_neg_0_4);
        } else if (!(placed >> (base + 1) & 1) && is_open(ctx, pos, nx, by + 1, nz) && !is_open(ctx, pos, nx, by, nz)) {
            level = 1;
            r = rot_make_floats(random_range_bound_0_25_in_block_in, random_range_bound_0_4_in_block_in);
        } else if (!(placed >> (base + 2) & 1) && is_open(ctx, pos, nx, by + 2, nz) && !is_open(ctx, pos, nx, by + 1, nz)) {
            level = 2;
            r = rot_make_floats(random_range_bound_0_25_in_block_in, random_range_bound_0_4_in_block_in);
        } else {
            continue;
        }

        // Aim at a random point on the top of the block below that spot.
        target->x = (0.5 - (double)(walk_update(block_in_rng_states_third, r, 0.5) * (float)dx)) + nx;
        target->z = (0.5 - (double)(walk_update(block_in_rng_states_fourth, r, 0.5) * (float)dz)) + nz;
        target->y = by + level;
        *bit = (unsigned char)(base + level);
        return true;
    }
    return false;
}

// Turn the head a step towards the target, and place when the aim is on it.
// Returns false if the module should turn itself off.
static bool aim_and_place(Ctx *ctx, Vec3 me, Vec3 target, unsigned char bit) {
    if (!update_rotation_priority_peak(500))
        return false;
    switch_best_hotbar_slot(ctx, block_in_module_descriptor, 1, key_category);
    if (!block_entity_id(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player)))
        return false;   // no blocks in the hotbar

    // How far the head still has to turn.
    Rot current = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
    Vec3 eye = java_get_vec3_x_coord(ctx->jvm, java_call_entity_living_base_get_position(ctx->jvm, ctx->player));
    Pair diff = rot_sub(rot_look_at(eye, target), current);
    float diff_yaw = wrap_angle(diff.x);
    float diff_pitch = diff.y;

    // Turn part of the way (by aim speed), limited to a random maximum per tick.
    Pair step = rot_scale(Pair{diff_yaw, diff_pitch}, (float)((double)block_in_aim_speed * 0.5 + 0.5));
    float jitter_yaw = walk_update(block_in_rng_states_fifth, rot_make_floats(random_range_bound_0_8_in_block_in, random_range_bound_1_2_in_block_in), 0.2);
    float jitter_pitch = walk_update(block_in_rng_states_in_block_in_d, rot_make_floats(random_range_bound_0_8_in_block_in, random_range_bound_1_2_in_block_in), 0.2);
    float max_yaw = jitter_yaw * 180.0f * block_in_aim_speed;
    float max_pitch = 180.0f * jitter_pitch * block_in_aim_speed;
    Pair limited = {limit_rotation_delta(-max_yaw, max_yaw, diff_yaw), limit_rotation_delta(-max_pitch, max_pitch, diff_pitch)};
    Pair turn = rot_lerp(step, limited, 0.5f);
    apply_silent_rotation(turn);

    // Within 1 degree after this step: right click.
    float error_yaw = abs_f(diff_yaw - turn.x);
    float error_pitch = abs_f(diff_pitch - turn.y);
    if (1.0f > error_pitch + error_yaw) {
        java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));
        placed_mask() |= 1ull << (bit & 63);
    }
    block_in_active = 1;

    // Not in the middle of the block yet: walk there first.
    double frac_x = abs_d(libc_fmod(me.x, 1.0));
    double frac_z = libc_fmod(me.z, 1.0);
    if (in_block_middle(frac_x, abs_d(frac_z))) {
        block_in_rotation_mode = 2;
        return true;
    }
    block_in_rotation_mode = 1;
    Vec3 middle = {libc_floor(me.x) + 0.5, me.y, libc_floor(me.z) + 0.5};
    block_in_placement_rotation = rot_look_at(me, middle);
    return true;
}

extern "C" void block_in_tick(Hook *hook) {
    block_in_active = 0;
    block_in_rotation_mode = 0;
    Ctx *ctx = hook->ctx;
    bool turn_off = false;

    if (ctx->in_game != 1) {
        turn_off = true;
    } else {
        unsigned char on_ground = java_get_entity_on_ground(ctx->jvm, ctx->player);
        if (block_in_require_ground && !on_ground) {
            turn_off = true;
        } else {
            Vec3 me = java_get_entity_pos_x(ctx->jvm, ctx->player);
            if (java_variant(ctx->jvm))
                me = vec3_add_eye_height_sneak(me);

            // The block below the feet.
            int bx = (int)libc_floor(me.x);
            int by = (int)(libc_floor(me.y) + -1.0);
            int bz = (int)libc_floor(me.z);

            // Moved up or down: start over.
            if (abs_d(me.y - block_in_last_y) > 0.01)
                placed_mask() = 0;
            block_in_last_y = me.y;

            // A BlockPos object reused for the block lookups (only once the JVM handle is ready).
            void *pos = **(int **)ctx->jvm ? java_block_pos_init(ctx->jvm, nullptr, 0, 0) : nullptr;

            Vec3 target;
            unsigned char bit;
            if (find_roof_spot(ctx, pos, bx, by, bz, &target, &bit) || find_wall_spot(ctx, pos, bx, by, bz, &target, &bit)) {
                turn_off = !aim_and_place(ctx, me, target, bit);
            } else {
                // Nothing left to place: done once we stand in the middle of the block.
                if (!on_ground)
                    return;
                double frac_x = abs_d(libc_fmod(me.x, 1.0));
                double frac_z = abs_d(libc_fmod(me.z, 1.0));
                turn_off = in_block_middle(frac_x, frac_z);
            }
        }
    }

    if (turn_off)
        queue_module_toggle(ctx->module_manager, block_in_module_descriptor, 0);
}
