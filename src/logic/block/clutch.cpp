// Clutch - save yourself from falling by placing a block.
//
// When you are about to fall (knocked off an edge, walking backwards into the void, ...),
// the module picks the best nearby block to place against, turns your head to it with
// smooth, slightly random movement, and right clicks once the crosshair is on it.
// Afterwards it turns your head back to where you were really looking ("snap back").
//
// Callbacks:
//   clutch_tick                    every tick: right click when the crosshair is on the planned block
//   clutch_rotation_override_flag  tells the engine we control the rotation this tick
//   clutch_on_late_tick            every tick: pick a block, aim at it, snap back afterwards
//   clutch_pick_aim_target         helper: aim point on one candidate block and how far we must turn
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct BlockPos { int x, y, z; };
struct Rot { float yaw, pitch; };
struct Box { Vec3 lo, hi; };
struct Range { float lo, hi; };

// ---- settings ----
extern "C" float clutch_clicks_per_second;
extern "C" float clutch_snapback_duration;          // ticks to turn back to the real rotation
extern "C" float clutch_snapback_delay;             // ticks to wait before turning back
extern "C" float clutch_randomisation;              // 0 = no randomness, 1 = full
extern "C" float clutch_aim_speed;                  // max degrees per tick
extern "C" float clutch_aim_multiplier;             // fraction of the remaining angle to turn per tick
extern "C" float clutch_aim_multiplier_strength;    // how much the multiplier is used vs. the max speed
extern "C" unsigned char clutch_snapback_instantly_on_jump;
extern "C" unsigned char clutch_require_air;        // only while in the air
extern "C" unsigned char clutch_require_damage;     // only within 10 ticks of being hit
extern "C" unsigned char clutch_require_moving_backwards;
extern "C" float clutch_min_height;                 // only when the ground is at least this far below
extern "C" float clutch_range;                      // how far away a block may be (index into the candidate table)
extern "C" float clutch_max_aim_distance;           // give up when the head would have to turn more than this
extern "C" unsigned char clutch_sideways_only;      // never place on the block straight below
extern "C" unsigned char clutch_multipoint_aim;
extern "C" int clutch_select_blocks_1_switch_2;     // 1 = switch to blocks, 2 = switch to blocks and back
extern "C" unsigned char clutch_disable_after;      // turn the module off after one clutch

// ---- module state ----
extern "C" char clutch_module_descriptor[];
extern "C" float clutch_click_cooldown;             // ticks until the next right click
extern "C" unsigned char clutch_planned;            // a block is planned and we hold blocks
extern "C" BlockPos clutch_planned_block;           // block to place against
extern "C" unsigned char clutch_planned_face_up;    // place on its top (1) or on a side (0)
extern "C" unsigned char clutch_rotating;           // we control the rotation this tick
extern "C" Vec3 clutch_aim_point;                   // point on the block we aim at
extern "C" int clutch_ticks_since_last_hurt;
extern "C" int clutch_ticks_left_jump_snapback;     // window after a clutch in which jumping snaps back at once
extern "C" int clutch_snapback_delay_second;        // ticks left before the snap back starts
extern "C" int clutch_snapback_ticks_left;          // ticks left of the snap back
extern "C" unsigned char clutch_disable_module_after_snapback;
extern "C" double clutch_jump_wiggle_direction;     // sum of the random yaw wiggles on jump
extern "C" char clutch_random_walk_1[], clutch_random_walk_2[], clutch_random_walk_3[], clutch_random_walk_4[],
                clutch_random_walk_5[], clutch_random_walk_6[], clutch_random_walk_7[], clutch_random_walk_8[];   // random walk states

// Candidate blocks around the player, nearest first. Each entry packs an offset as three
// 4-bit numbers (x, y, z, each + 8). The first table says how many entries are within a distance.
extern "C" const unsigned short clutch_candidates_within_distance_n[8];
extern "C" const unsigned short clutch_candidate_offsets_4_bits_each[];

// Bounds of random ranges (the name says the value: neg_0_1 = -0.1, 0_05 = 0.05).
extern "C" const float random_range_bound_0_05_in_clutch, random_range_bound_neg_0_1_in_clutch, random_range_bound_0_1_in_clutch, random_range_bound_neg_20_in_clutch,
                       random_range_bound_20_in_clutch_a, random_range_bound_neg_0_05_in_clutch, random_range_bound_5_in_clutch, random_range_bound_15_in_clutch,
                       random_range_bound_0_05_in_clutch_a, random_range_bound_neg_0_01, random_range_bound_0_01_in_clutch;

// ---- game / engine functions ----
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 1 = a block
FN(BlockPos, java_get_moving_object_position_block_pos, (void *jvm, void *hit));
FN(int, java_get_variant, (void *jvm, void *hit));                                    // face hit, 1 = top
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));     // = one right click
FN(Vec3, java_get_adjacent_block_position, (void *jvm, void *hit));                  // where the new block goes
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(unsigned char, is_silent_rotation_active, ());
FN(void *, get_held_item, (void *jvm, void *game_settings));                          // here: the jump key
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(Rot, get_silent_rotation_offset, ());
FN(Rot, get_render_rotation, ());                                                     // where the player really looks
FN(Rot, rot_add, (Rot a, Rot b));
FN(Rot, rot_sub, (Rot a, Rot b));
FN(Rot, rot_scale, (Rot rotation, float k));
FN(float, wrap_angle, (float deg));                                                   // to -180..180
FN(void, apply_silent_rotation, (Rot rotation));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, random_rotation, (Range r));                                                // random value in the range
FN(float, walk_update, (void *state, Range r, double d));                            // next value of a random walk
FN(float, walk_update_timed, (void *state, Range r, double a, double b));
FN(float, limit_rotation_delta, (float lo, float hi, float v));                       // clamp
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(void *, java_call_entity_living_base_get_position, (void *jvm, void *entity_living_base));   // eye position
FN(Vec3, java_get_vec3_x_coord, (void *jvm, void *vec3));
FN(void *, java_get_entity_riding_entity, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(double, libc_hypot, (double x, double y));
FN(double, libc_acos, (double x));
FN(float, libc_sinf, (float x));
FN(float, libc_cosf, (float x));
FN(double, libc_floor, (double x));
FN(double, libc_fmod, (double x, double y));
FN(void *, java_block_pos_init, (void *jvm, void *object, int y, int z));
FN(void *, java_call_world_get_block_state, (void *jvm, void *world, void *pos, int x, int y, int z));
FN(int, get_block_id, (void *jvm, void *block));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(void *, java_new_vec3_init, (void *jvm, Vec3 v));
FN(void *, java_world_ray_trace_blocks, (void *jvm, void *world, void *from, void *to, unsigned char liquids, unsigned char ignore_no_box, unsigned char last));
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Rot, rot_look_at, (Vec3 from, Vec3 to));
FN(void, track_owner, (Ctx *ctx, void *mod, unsigned char back));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(unsigned char, block_entity_id, (void *jvm, void *entity));                        // 1 if the item is a block
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));
FN(unsigned char, update_rotation_priority_peak, (int ms));                          // may this module control the rotation?
FN(Vec3, java_get_entity_rotation_pitch, (void *jvm, void *entity));                  // look direction (unit vector)
FN(Vec3, vec3_scale, (Vec3 v, double k));
FN(unsigned char, aabb_ray_hit, (Box b, Vec3 origin, Vec3 dir, Vec3 *hit));

static const float BIGGEST_FLOAT = 3.40282347e+38f;
static const double MAX_REACH_SQUARED = 4.49 * 4.49;

static int abs_i(int v) { return v < 0 ? -v : v; }
static double abs_d(double x) { return __builtin_fabs(x); }
static float abs_f(float x) { return __builtin_fabsf(x); }

static bool same_block(BlockPos a, BlockPos b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

// Every tick: right click when the crosshair is on the planned block, at "clicks_per_second".
extern "C" void clutch_tick(Hook *hook) {
    float cooldown = clutch_click_cooldown;
    if (cooldown > 0.0f) {
        cooldown += -1.0f;
        clutch_click_cooldown = cooldown;
        if (cooldown > 0.0f)
            return;
    }
    clutch_click_cooldown = 20.0f / clutch_clicks_per_second + cooldown;

    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1 || !clutch_planned)
        return;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 1)
        return;
    if (!same_block(java_get_moving_object_position_block_pos(ctx->jvm, hit), clutch_planned_block))
        return;
    // Must be the planned face: the top, or a side.
    bool hit_top = java_get_variant(ctx->jvm, hit) == 1;
    if (hit_top != (clutch_planned_face_up != 0))
        return;

    java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));

    // Next time, place against the block we just placed.
    Vec3 placed = java_get_adjacent_block_position(ctx->jvm, hit);
    clutch_planned_block.x = (int)placed.x;
    clutch_planned_block.y = (int)placed.y;
    clutch_planned_block.z = (int)placed.z;
}

extern "C" void clutch_rotation_override_flag(char *h) {
    if (clutch_rotating == 1) {
        h[8] = 1;   // override yaw
        h[9] = 1;   // override pitch
    }
}

// Pick a point on the side of a candidate block (at base + rel) that faces the player.
// Writes it to *out and returns how many degrees the head has to turn to look at it.
extern "C" float clutch_pick_aim_target(Vec3 *eye, Rot cur, Vec3 *center, BlockPos *base, BlockPos *rel, Vec3 *out) {
    int rx = rel->x, ry = rel->y, rz = rel->z;
    float jitter_inset = walk_update(clutch_random_walk_1, rot_make_floats(0.0f, random_range_bound_0_05_in_clutch_a), 1.0);
    float randomness = clutch_randomisation;
    float jitter_x = walk_update(clutch_random_walk_2, rot_make_floats(random_range_bound_neg_0_01, random_range_bound_0_01_in_clutch), 0.5);
    float jitter_z = walk_update(clutch_random_walk_3, rot_make_floats(random_range_bound_neg_0_01, random_range_bound_0_01_in_clutch), 0.5);
    double shift_x = (double)(jitter_x * randomness), shift_z = (double)(jitter_z * randomness);

    if (!clutch_multipoint_aim) {
        // Lean towards the side of the block the previous aim point was on.
        double frac_x = abs_d(libc_fmod(center->x, 1.0));
        double frac_z = abs_d(libc_fmod(center->z, 1.0));
        frac_x = 0.5 < frac_x ? abs_d(frac_x + -1.0) : frac_x;
        frac_z = 0.5 < frac_z ? abs_d(frac_z + -1.0) : frac_z;
        shift_z += frac_x * (clutch_aim_point.z < center->z ? -1.0 : 1.0);
        shift_x += frac_z * (clutch_aim_point.x < center->x ? -1.0 : 1.0);
    }

    // The face pointing at us: on the x side if the block is further away in x, else on the z side.
    // Stay a little away from the face's edges.
    double inset = (double)(randomness * jitter_inset) + 0.1;
    double far = 1.0 - inset;
    double x_lo, x_hi, z_lo, z_hi;
    if ((unsigned)abs_i(rx) > (unsigned)abs_i(rz)) {
        x_lo = x_hi = (double)((unsigned)rx >> 31);   // 1 if the block is at -x (aim at its +x face), else 0
        z_lo = inset;
        z_hi = far;
    } else {
        x_lo = inset;
        x_hi = far;
        z_lo = z_hi = (double)((unsigned)rz >> 31);
    }

    // Aim point: as close to our position as the face allows; higher for blocks further away.
    int bx = rx + base->x, by = ry + base->y, bz = rz + base->z;
    double distance = (double)(abs_i(rel->x) + abs_i(rel->z));
    double aim_x = limit_rotation_delta((float)(x_lo + bx), (float)(x_hi + bx), (float)(shift_x + center->x));
    double aim_y = (double)limit_rotation_delta(0.0f, 0.65f, (float)(distance * 0.25)) + ((double)by + 0.25);
    double aim_z = limit_rotation_delta((float)(z_lo + bz), (float)(z_hi + bz), (float)(shift_z + center->z));
    Vec3 aim = vec3_make(aim_x, aim_y, aim_z);

    Rot diff = rot_sub(rot_look_at(*eye, aim), cur);
    *out = aim;
    return abs_f(wrap_angle(diff.yaw)) + abs_f(diff.pitch);
}

// Should we try to clutch right now?
static bool should_clutch(Ctx *ctx, unsigned char on_ground) {
    if (clutch_require_air & on_ground)
        return false;
    if (clutch_ticks_since_last_hurt >= 11 && clutch_require_damage)
        return false;
    if (java_get_entity_riding_entity(ctx->jvm, ctx->player))
        return false;
    if (clutch_require_moving_backwards == 1) {
        Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
        double speed = libc_hypot(motion.x, motion.z);
        if (speed == 0.0)
            return false;
        // Angle between the facing direction and the motion: under 90 degrees = moving forwards.
        float facing = (float)((double)get_entity_rotation(ctx->jvm, ctx->player).yaw * -3.141592653589793 / 180.0 + -3.141592653589793);
        float sin_facing = libc_sinf(facing), cos_facing = libc_cosf(facing);
        if (1.5707963267948966 > libc_acos(((double)sin_facing * motion.x + (double)cos_facing * motion.z) / speed))
            return false;
    }
    return true;
}

// Find the best block to place against. Sets clutch_aim_point / clutch_planned_block.
// Returns false when there is nothing to do.
static bool plan_block(Ctx *ctx, Box player_box, Vec3 eye, Rot cur) {
    // Middle of the feet, just inside the block below.
    Vec3 center = {(player_box.lo.x + player_box.hi.x) * 0.5, player_box.lo.y + -0.99, (player_box.lo.z + player_box.hi.z) * 0.5};
    clutch_planned_face_up = 0;
    void *pos = **(int **)ctx->jvm ? java_block_pos_init(ctx->jvm, nullptr, 0, 0) : nullptr;
    BlockPos base = {(int)libc_floor(center.x), (int)libc_floor(center.y), (int)libc_floor(center.z)};

    // Ground closer than "min_height": nothing to do.
    if (clutch_min_height > 0.0f) {
        int y = base.y;
        for (int depth = 1;; depth++, y--) {
            if (get_block_id(ctx->jvm, java_call_world_get_block_state(ctx->jvm, ctx->world, pos, base.x, y, base.z)))
                return false;
            if (!(clutch_min_height > (float)depth))
                break;
        }
    }

    unsigned long long candidates = clutch_candidates_within_distance_n[(int)clutch_range];
    long long best_rank = -3;
    float best_cost = BIGGEST_FLOAT;
    Vec3 best_aim = {};
    BlockPos best_block = {};
    Vec3 aim;

    // Keep the previously planned block while it is still there, reachable and visible.
    if (!((double)clutch_planned_block.y > center.y) &&
        get_block_id(ctx->jvm, java_call_world_get_block_state(ctx->jvm, ctx->world, pos, clutch_planned_block.x, clutch_planned_block.y, clutch_planned_block.z))) {
        BlockPos rel = {clutch_planned_block.x - base.x, clutch_planned_block.y - base.y, clutch_planned_block.z - base.z};
        float cost = clutch_pick_aim_target(&eye, cur, &center, &base, &rel, &aim);
        if (!(cost > clutch_max_aim_distance) && MAX_REACH_SQUARED > vec3_distance_sq(eye, aim)) {
            void *hit = java_world_ray_trace_blocks(ctx->jvm, ctx->world, java_new_vec3_init(ctx->jvm, eye), java_new_vec3_init(ctx->jvm, aim), 0, 1, 0);
            BlockPos hit_block = clutch_planned_block;
            if (hit)
                hit_block = java_get_moving_object_position_block_pos(ctx->jvm, hit);
            if (same_block(hit_block, clutch_planned_block)) {
                best_aim = aim;
                best_block = hit_block;
                best_cost = BIGGEST_FLOAT;
                if (cost != BIGGEST_FLOAT) {
                    best_rank = abs_i(hit_block.x - base.x) + abs_i(hit_block.y - base.y) + abs_i(hit_block.z - base.z) - 2;
                    best_cost = cost;
                }
            }
        }
    }

    // Try the candidate blocks, nearest first.
    for (unsigned long long i = 0; i < candidates; i++) {
        unsigned packed = clutch_candidate_offsets_4_bits_each[i];
        int ox = (packed & 0xf) - 8, oy = (packed >> 4 & 0xf) - 8, oz = (packed >> 8 & 0xf) - 8;
        int x = base.x + ox, y = base.y + oy, z = base.z + oz;
        if (!get_block_id(ctx->jvm, java_call_world_get_block_state(ctx->jvm, ctx->world, pos, x, y, z)))
            continue;   // air, nothing to place against

        if (!ox && !oz) {
            // A block straight below: aim at a random point on its top. That always wins.
            if (clutch_sideways_only)
                return false;
            double inset = (double)(walk_update(clutch_random_walk_1, rot_make_floats(0.0f, random_range_bound_0_05_in_clutch), 1.0) * clutch_randomisation) + 0.1;
            double shift_x = walk_update(clutch_random_walk_4, rot_make_floats(random_range_bound_neg_0_1_in_clutch, random_range_bound_0_1_in_clutch), 1.0) * clutch_randomisation;
            double shift_z = walk_update(clutch_random_walk_5, rot_make_floats(random_range_bound_neg_0_1_in_clutch, random_range_bound_0_1_in_clutch), 1.0) * clutch_randomisation;
            double aim_x = limit_rotation_delta((float)((double)x + inset), (float)((double)(x + 1) - inset), (float)(center.x + shift_x));
            double aim_z = limit_rotation_delta((float)((double)z + inset), (float)((double)(z + 1) - inset), (float)(center.z + shift_z));
            clutch_aim_point = vec3_make(aim_x, (double)y + 1.0, aim_z);
            clutch_planned_block = BlockPos{x, y, z};
            clutch_planned_face_up = 1;
            return true;
        }

        // Once a block is found, only blocks at the same distance are still worth checking.
        long long rank = abs_i(ox) + abs_i(oy) + abs_i(oz);
        candidates = clutch_candidates_within_distance_n[rank];
        BlockPos rel = {ox, oy, oz};
        float cost = clutch_pick_aim_target(&eye, cur, &center, &base, &rel, &aim);
        if (cost > best_cost && best_rank <= rank)
            continue;
        if (!(MAX_REACH_SQUARED > vec3_distance_sq(eye, aim)))
            continue;
        best_aim = aim;
        best_block = BlockPos{x, y, z};
        best_cost = cost;
    }

    if (best_cost > clutch_max_aim_distance)
        return false;
    clutch_aim_point = best_aim;
    clutch_planned_block = best_block;
    return true;
}

// Turn the head a step towards the aim point.
static void aim_at_planned_block(Ctx *ctx, Vec3 eye, Rot cur) {
    // Already looking at the top of the planned block: nothing to do.
    int bx = clutch_planned_block.x, by = clutch_planned_block.y, bz = clutch_planned_block.z;
    Vec3 look = vec3_scale(java_get_entity_rotation_pitch(ctx->jvm, ctx->player), 4.4);
    Box top = {{bx + 0.02, by + 0.02, bz + 0.02}, {bx + 0.98, (double)(by + 1), 0.98 + bz}};
    Vec3 hit;
    if (aabb_ray_hit(top, eye, look, &hit) && (double)by + 0.99 > hit.y)
        return;

    Rot want = rot_look_at(eye, clutch_aim_point);
    Rot diff = rot_sub(want, cur);
    float yaw_diff;
    if ((double)want.pitch > 89.999)
        yaw_diff = walk_update(clutch_random_walk_6, rot_make_floats(random_range_bound_neg_20_in_clutch, random_range_bound_20_in_clutch_a), 0.5) * clutch_randomisation;   // straight down: yaw does not matter, wiggle it
    else
        yaw_diff = wrap_angle(diff.yaw);

    // Turn at most "aim_speed" (randomly a bit less) per tick...
    float speed = clutch_aim_speed;
    Range speed_range = rot_make_floats((float)((double)clutch_randomisation * -0.25 + 1.0), 1.0f);
    float max_yaw = walk_update_timed(clutch_random_walk_7, speed_range, 10.0, 1.0) * speed;
    speed_range = rot_make_floats((float)((double)clutch_randomisation * -0.25 + 1.0), 1.0f);
    float max_pitch = speed * walk_update_timed(clutch_random_walk_8, speed_range, 10.0, 1.0);
    float capped_yaw = limit_rotation_delta(-max_yaw, max_yaw, yaw_diff);
    float capped_pitch = limit_rotation_delta(-max_pitch, max_pitch, diff.pitch);

    // ...mixed with turning a fraction ("aim_multiplier") of the remaining angle.
    float mult_yaw = clutch_aim_multiplier + walk_update_timed(clutch_random_walk_7, rot_make_floats(random_range_bound_neg_0_05_in_clutch, random_range_bound_0_05_in_clutch), 10.0, 1.0);
    float mult_pitch = clutch_aim_multiplier + walk_update_timed(clutch_random_walk_8, rot_make_floats(random_range_bound_neg_0_05_in_clutch, random_range_bound_0_05_in_clutch), 10.0, 1.0);
    float turn_yaw = lerp_unclamped_float(capped_yaw, mult_yaw * yaw_diff, clutch_aim_multiplier_strength);
    float turn_pitch = lerp_unclamped_float(capped_pitch, mult_pitch * diff.pitch, clutch_aim_multiplier_strength);
    apply_silent_rotation(Rot{turn_yaw, turn_pitch});
}

// Turn back to the real rotation, evenly over the remaining snap-back ticks.
static void snap_back(Ctx *ctx) {
    if (clutch_snapback_delay_second > 0)
        return;
    int ticks_left = clutch_snapback_ticks_left;
    if (ticks_left <= 0 || !is_silent_rotation_active())
        return;
    Rot now = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
    Rot diff = rot_sub(get_render_rotation(), now);
    diff.yaw = wrap_angle(diff.yaw);
    apply_silent_rotation(rot_scale(diff, 1.0f / (float)ticks_left));
}

extern "C" void clutch_on_late_tick(Hook *hook) {
    clutch_planned = 0;
    clutch_rotating = 0;
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    unsigned char on_ground = java_get_entity_on_ground(ctx->jvm, ctx->player);
    clutch_ticks_since_last_hurt = java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) == 0 ? clutch_ticks_since_last_hurt + 1 : 0;

    // Shortly after a clutch, jumping turns the head back at once (with a small random wiggle).
    if (clutch_ticks_left_jump_snapback > 0) {
        clutch_ticks_left_jump_snapback -= 1;
        if (on_ground && is_silent_rotation_active() && clutch_snapback_instantly_on_jump &&
            java_get_key_binding_key_code(ctx->jvm, get_held_item(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)))) {
            Rot cur = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
            Rot real = get_render_rotation();
            double sign = 0.0 > clutch_jump_wiggle_direction ? 1.0 : -1.0;   // wiggle the other way than last time
            float wiggle_yaw = (float)((double)random_rotation(rot_make_floats(random_range_bound_5_in_clutch, random_range_bound_15_in_clutch)) * sign);
            float wiggle_pitch = (float)((double)random_rotation(rot_make_floats(random_range_bound_5_in_clutch, random_range_bound_15_in_clutch)) * sign);
            clutch_jump_wiggle_direction += wiggle_yaw;
            Rot diff = rot_sub(rot_add(real, rot_scale(Rot{wiggle_yaw, wiggle_pitch}, clutch_randomisation)), cur);
            diff.yaw = wrap_angle(diff.yaw);
            apply_silent_rotation(diff);
            clutch_rotating = 1;
            clutch_snapback_delay_second = (int)clutch_snapback_duration;
            return;
        }
    }

    Box player_box = java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, ctx->player));
    Vec3 eye = java_get_vec3_x_coord(ctx->jvm, java_call_entity_living_base_get_position(ctx->jvm, ctx->player));
    Rot cur = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());

    bool start_aiming = false;
    if (should_clutch(ctx, on_ground) && plan_block(ctx, player_box, eye, cur)) {
        if (clutch_select_blocks_1_switch_2 == 1)
            track_owner(ctx, clutch_module_descriptor, 0);
        else if (clutch_select_blocks_1_switch_2 == 2)
            track_owner(ctx, clutch_module_descriptor, 1);
        clutch_planned = block_entity_id(ctx->jvm, item_held_hotbar_slot(ctx->jvm, ctx->player));
        if (clutch_planned) {
            // Plan the snap back for after the clutch.
            float delay_scale = random_rotation(rot_make_floats(1.0f, (float)((double)clutch_randomisation * 0.5 + 1.0)));
            clutch_snapback_delay_second = (int)(delay_scale * clutch_snapback_delay);
            clutch_ticks_left_jump_snapback = 10;
            clutch_disable_module_after_snapback = clutch_disable_after;
            start_aiming = true;
        }
    } else {
        clutch_planned = 0;
    }

    if (!start_aiming) {
        // Not clutching: run the snap-back timers.
        if (clutch_snapback_delay_second > 0) {
            clutch_snapback_delay_second -= 1;
            clutch_snapback_ticks_left = (int)clutch_snapback_duration;
        } else if (clutch_snapback_ticks_left > 0) {
            clutch_snapback_ticks_left -= 1;
            clutch_planned = 0;
        } else {
            // Snap back finished.
            if (clutch_disable_module_after_snapback == 1) {
                queue_module_toggle(ctx->module_manager, clutch_module_descriptor, 0);
                clutch_disable_module_after_snapback = 0;
            }
            return;
        }
    }

    if (!update_rotation_priority_peak(1000))
        return;
    clutch_rotating = 1;
    if (clutch_planned)
        aim_at_planned_block(ctx, eye, cur);
    else
        snap_back(ctx);
}
