// Aim Assist - turn your view towards the nearest suitable player.
//
// Every tick the module picks a target (a player in range, inside the "fov", not a friend,
// sorted by "sort_by") and a point on it, moved around a little with smooth noise so the
// aim does not sit on one spot. Every frame it turns the view a step towards that point.
//   mode 0: smooth turn       mode 1: turn at most a fixed speed
//   mode 2: instant (yaw only, unless "lock_on_vertical")   mode 3: silent (only the server sees it)
//
// Callbacks:
//   aim_assist_disable               module switched off: forget the target
//   aim_assist_on_rotation_update    rotation hook: tell the engine we control the rotation
//   aim_assist_tick                  every tick: choose / keep the target and its aim point
//   aim_assist_frame                 every frame: turn towards the aim point
//   aim_assist_apply_frame_rotation  helper: one frame of turning (deadzone, speed, randomness)
//   aim_assist_test_entity_updating  helper: check one player, and keep it if it is the best so far
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Box { Vec3 lo, hi; };
struct Range { float lo, hi; };

struct FrameEvt {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    float partial;       // +0x10 how far we are between two ticks (0..1)
    int padding_14;
    double dt;           // +0x18 seconds since the last frame
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" int aim_assist_mode;                          // see above (0..3)
extern "C" unsigned char aim_assist_lock_on_vertical;    // mode 2: turn the pitch too
extern "C" unsigned char aim_assist_require_mouse_moved; // never turn faster than the mouse moves
extern "C" unsigned char aim_assist_silent_ignore_manual_aim;   // mode 3
extern "C" unsigned char aim_assist_silent_move_correct_value;  // mode 3: move in the direction the server sees
extern "C" unsigned char aim_assist_require_sprint;
extern "C" unsigned char aim_assist_require_mouse_pressed;
extern "C" unsigned char aim_assist_disable_on_block_break;     // not while mining
extern "C" unsigned char aim_assist_require_weapon;
extern "C" int aim_assist_mode_target;                   // 0 = keep the target while valid, else re-pick every tick
extern "C" unsigned char aim_assist_ignore_invisible;
extern "C" unsigned char aim_assist_allow_exceeding_fov; // keep aiming at a kept target that left the fov
extern "C" unsigned char aim_assist_require_line_sight;
extern "C" int aim_assist_sort_by_distance_health_hurt;  // "sort_by": 0 distance, 1 health, 2 hurt time, 3 fov
extern "C" float aim_assist_deadzone;                    // degrees close enough not to turn
extern "C" float aim_assist_fov;                         // degrees
extern "C" float aim_assist_horizontal_vertical_speed, aim_assist_horizontal_vertical_speed_second;   // horizontal / vertical speed
extern "C" float aim_assist_near_distance;               // don't aim at targets closer than this
extern "C" float aim_assist_range;
extern "C" float aim_assist_predict;                     // 0..1: aim between the shown and the server position
extern "C" float aim_assist_horizontal_vertical_hitbox_correction, aim_assist_horizontal_vertical_hitbox_correction_second;   // aim at the near side of the hitbox (h / v)
extern "C" float aim_assist_aim_randomisation;           // size of the aim point noise

// ---- module state ----
extern "C" char aim_assist_module_descriptor[];
extern "C" unsigned char aim_assist_has_target;          // there is a target to aim at
extern "C" void *aim_assist_cached_target_ref;           // the current target (JNI global ref)
extern "C" unsigned char aim_assist_aiming;              // we control the rotation (until handed back)
extern "C" Vec3 aim_assist_previous_current_aim_point;          // aim point last tick
extern "C" Vec3 aim_assist_previous_current_aim_point_second;   // aim point this tick
extern "C" Rot aim_assist_mouse_movement_since_last_tick;       // how far the player moved the mouse (yaw, pitch)
extern "C" int aim_assist_clicks_wait_before_aiming;
extern "C" unsigned long long aim_assist_tie_break_counter;
extern "C" char aim_assist_rng_states[], aim_assist_rng_states_second[], aim_assist_rng_states_third[], aim_assist_rng_states_fourth[], aim_assist_rng_states_fifth[];   // random walk states

// ---- game / engine functions ----
FN(unsigned char, update_rotation_priority_peak, (int ms));     // may this module control the rotation?
FN(unsigned char, is_silent_rotation_active, ());
FN(Rot, get_render_rotation, ());                                // where the player really looks
FN(Rot, get_silent_rotation_offset, ());
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(Rot, rot_add, (Rot a, Rot b));
FN(Rot, rot_sub, (Rot a, Rot b));
FN(Rot, rot_scale, (Rot rotation, float k));
FN(float, rot_length, (Rot rotation));
FN(float, wrap_angle, (float deg));                              // to -180..180
FN(void, apply_silent_rotation, (Rot rotation));                 // turn by this much
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height, (Vec3 v));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(Vec3, vec3_scale, (Vec3 v, double k));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Rot, rot_look_at, (Vec3 from, Vec3 to));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double d));       // next value of a random walk
FN(float, limit_rotation_delta, (float lo, float hi, float v)); // clamp
FN(unsigned char, random_chance, (float chance));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));          // a mouse button is held
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *hit));   // 1 = a block
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(unsigned char, java_get_entity_is_dead, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_player_sleeping, (void *jvm, void *entity_player));
FN(unsigned char, java_call_entity_player_is_invisible_to_player, (void *jvm, void *entity_player, void *viewer));
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int checks));      // 7: friend, teammate or bot
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));   // server position
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(Box, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(Box, aabb_expand, (Box b, double grow));
FN(Vec3, aabb_clamp_point, (Box b, Vec3 p));                     // closest point of the box
FN(double, vec3_distance, (Vec3 a, Vec3 b));
FN(double, lerp_double, (double a, double b, double t));
FN(float, noise_1d, (float x));                                  // smooth noise, -1..1
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(unsigned char, set_vertex_pointer, (void *jvm, void *world, Vec3 from, Vec3 to));   // clear line of sight
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));

static const float BIGGEST_FLOAT = 3.40282347e+38f;

static Rot wrap(Rot rotation) {
    rotation.yaw = wrap_angle(rotation.yaw);
    return rotation;
}

static void delete_global_ref(void *jvm, void *ref) {
    JNIEnv *env = ((Jvm *)jvm)->env;
    (*env)->DeleteGlobalRef(env, ref);
}

// On disable: forget the target.
extern "C" void aim_assist_disable(EvHook *hook) {
    if (hook->ev != aim_assist_module_descriptor || hook->on)
        return;
    void *ref = aim_assist_cached_target_ref;
    if (ref)
        delete_global_ref(hook->ctx->jvm, ref);
    aim_assist_cached_target_ref = nullptr;
    aim_assist_has_target = 0;
}

extern "C" void aim_assist_on_rotation_update(char *h) {
    if (aim_assist_aiming == 1) {
        h[8] = 1;   // we control the yaw
        if (aim_assist_silent_move_correct_value == 1)
            h[9] = 1;
    }
}

// Check one player as a target. If it is better than the best so far (by the "sort_by" score),
// update the scores and write its aim point; returns 1. `sticky`: it is the kept target.
// *should_aim_out: 0 when the target is closer than "near_distance" (no need to aim).
extern "C" unsigned char aim_assist_test_entity_updating(Ctx *c, void *e, Vec3 *eye, Rot rot, unsigned char sticky, double *range,
                                     float *best_hp, int *best_hurt, unsigned *best_tie, float *fov,
                                     Vec3 *aim_out, unsigned char *should_aim_out) {
    if (java_get_entity_is_dead(c->jvm, e) || java_get_entity_player_sleeping(c->jvm, e))
        return 0;
    if (aim_assist_ignore_invisible == 1 && java_call_entity_player_is_invisible_to_player(c->jvm, e, c->player))
        return 0;
    if (entity_flags_has(c->jvm, e, 7))
        return 0;

    // Distance to the nearest point of its hitbox (grown by 0.1).
    Vec3 pos = java_get_entity_pos_x(c->jvm, e);
    if (aim_assist_predict > 0.0f)
        pos = vec3_lerp(pos, java_get_entity_other_player_mp_other_player_mpx(c->jvm, e), aim_assist_predict);
    Box hitbox = aabb_expand(java_get_axis_aligned_bb_min_x(c->jvm, java_get_entity_bounding_box(c->jvm, e)), (double)0.1f);
    Vec3 nearest = aabb_clamp_point(hitbox, *eye);
    double distance = vec3_distance(*eye, nearest);
    unsigned char should_aim = !((double)aim_assist_near_distance > distance);
    if (distance > *range)
        return 0;

    // Aim point: its eyes, pulled towards the nearest hitbox point by the hitbox correction
    // (less correction closer than 3.5 blocks).
    Vec3 head = vec3_add_eye_height(pos);
    double aim_x, aim_z;
    float correction = aim_assist_horizontal_vertical_hitbox_correction;
    if (correction > 0.0f) {
        if (3.5 > distance)
            correction = (float)(correction * (distance / 3.5));
        Vec3 pulled = vec3_lerp(head, nearest, correction);
        aim_x = pulled.x;
        aim_z = pulled.z;
    } else {
        aim_x = head.x;
        aim_z = head.z;
    }
    double aim_y = lerp_double(head.y, nearest.y, aim_assist_horizontal_vertical_hitbox_correction_second);

    // Move it around with smooth noise (seeded by the positions, so it drifts as things move).
    float seed = (float)((eye->x + eye->y + eye->z + aim_x + aim_y + aim_z) * 0.1);
    float seed_2 = 10000.0f + seed, seed_3 = 20000.0f + seed;
    float noise_x = noise_1d(seed), noise_y = noise_1d(seed_2);
    double noise_z = (double)noise_1d(seed_3) * 0.5;
    float randomness = aim_assist_aim_randomisation;
    if (3.5 > distance)
        randomness = (float)(randomness * (distance / 3.5 * 0.9 + 0.1));
    Vec3 jitter = vec3_scale(Vec3{(double)noise_x * 0.5, (double)noise_y * 0.5, noise_z}, randomness);
    Vec3 aim = vec3_add(Vec3{aim_x, aim_y, aim_z}, jitter);

    // How far off our view it is (yaw).
    float off_angle = __builtin_fabsf(wrap_angle(rot_sub(rot_look_at(*eye, aim), rot).yaw));
    if (off_angle > *fov) {
        if (!sticky)
            return 0;
        should_aim = aim_assist_allow_exceeding_fov;
    }

    float health = java_call_entity_living_base_get_health(c->jvm, e);
    if (health > *best_hp)
        return 0;
    int hurt = java_get_entity_living_base_hurt_time(c->jvm, e);
    if (hurt > *best_hurt)
        return 0;
    if (aim_assist_require_line_sight == 1 && !set_vertex_pointer(c->jvm, c->world, *eye, aim))
        return 0;

    // Better than the best so far: store its score.
    if (!aim_assist_mode_target) {
        *fov = off_angle;
    } else {
        switch (aim_assist_sort_by_distance_health_hurt) {
        case 0:
            *range = distance;
            break;
        case 1:
            *best_hp = health;
            break;
        case 2: {
            // Same hurt time: break the tie with a hash of the id.
            unsigned hash = java_get_entity_entity_id(c->jvm, e) + (unsigned)aim_assist_tie_break_counter;
            hash = (hash >> 16 ^ hash) * 0x45d9f3b;
            hash = (hash >> 16 ^ hash) * 0x45d9f3b;
            hash = hash >> 16 ^ hash;
            aim_assist_tie_break_counter += 1;
            if (hurt == *best_hurt && hash < *best_tie)
                return 0;
            *best_hurt = hurt;
            *best_tie = hash;
            break;
        }
        case 3:
            *fov = off_angle;
            break;
        }
    }
    *aim_out = aim;
    *should_aim_out = should_aim;
    return 1;
}

// One frame of turning by (at most) `diff`, `dt` seconds long.
extern "C" void aim_assist_apply_frame_rotation(Rot diff, float dt) {
    float yaw = diff.yaw, pitch = diff.pitch;

    // Deadzone: close enough does not need turning.
    const float deadzone = aim_assist_deadzone;
    if (yaw > 0.0f) {
        yaw -= deadzone;
        yaw = yaw > 0.0f ? yaw : 0.0f;
    } else {
        yaw += deadzone;
        yaw = yaw < 0.0f ? yaw : 0.0f;
    }
    if (pitch > 0.0f) {
        pitch -= deadzone;
        pitch = pitch > 0.0f ? pitch : 0.0f;
    } else {
        pitch += deadzone;
        pitch = pitch < 0.0f ? pitch : 0.0f;
    }

    if (aim_assist_mode == 2) {
        // Instant.
        if (!aim_assist_lock_on_vertical)
            pitch = 0.0f;
    } else {
        // The pitch follows less while the yaw is still far off (fully off at 40-120 degrees).
        float full_off_at = walk_update(aim_assist_rng_states, rot_make_floats(40.0f, 120.0f), 5.0);
        float pitch_factor = (float)(-1.0 / (double)full_off_at * (double)__builtin_fabsf(yaw) + 1.0);
        pitch *= pitch_factor > 0.0f ? pitch_factor : 0.0f;

        float speed_h = aim_assist_horizontal_vertical_speed, speed_v = aim_assist_horizontal_vertical_speed_second;
        if (aim_assist_mode == 3) {
            // Silent: at least 5.
            speed_h = 5.0f > speed_h ? 5.0f : speed_h;
            speed_v = 5.0f > speed_v ? 5.0f : speed_v;
        }
        speed_h *= 0.5f;
        speed_v *= 0.5f;
        // Speeds squared, randomly +-10%, and a slow +-5% drift.
        float rate_yaw = walk_update(aim_assist_rng_states_third, rot_make_floats(0.9f, 1.1f), 1.0) * (speed_h * speed_h);
        float rate_pitch = walk_update(aim_assist_rng_states_fourth, rot_make_floats(0.9f, 1.1f), 1.0) * (speed_v * speed_v);
        float drift = walk_update(aim_assist_rng_states_fifth, Range{0.95f, 1.05f}, 60.0);

        if (aim_assist_mode == 1) {
            // At most rate * 5 degrees per second.
            float max_yaw = rate_yaw * 5.0f * drift * dt;
            yaw = limit_rotation_delta(-max_yaw, max_yaw, yaw);
            float max_pitch = rate_pitch * 5.0f * drift * dt;
            pitch = limit_rotation_delta(-max_pitch, max_pitch, pitch);
        } else {
            // A fraction of the remaining angle (never overshooting).
            Rot step = rot_scale(rot_scale(Rot{rate_yaw * yaw, rate_pitch * pitch}, drift), dt);
            if (!(__builtin_fabsf(step.yaw) > __builtin_fabsf(yaw)))
                yaw = step.yaw;
            if (!(__builtin_fabsf(step.pitch) > __builtin_fabsf(pitch)))
                pitch = step.pitch;
        }
        // A little vertical wobble while turning sideways.
        if (aim_assist_mode != 2) {
            float wobble = (float)((double)__builtin_fabsf(yaw) * 0.1);
            pitch += walk_update(aim_assist_rng_states_second, rot_make_floats(-wobble, wobble), 1.0);
        }
    }

    if (aim_assist_require_mouse_moved == 1) {
        // Never turn more than the player moved the mouse since last tick.
        float abs_yaw = __builtin_fabsf(yaw), abs_pitch = __builtin_fabsf(pitch);
        float mouse_yaw = __builtin_fabsf(aim_assist_mouse_movement_since_last_tick.yaw);
        float mouse_pitch = __builtin_fabsf(aim_assist_mouse_movement_since_last_tick.pitch);
        bool cut_yaw = mouse_yaw < abs_yaw && yaw != 0.0f;
        float cut_yaw_value = mouse_yaw / abs_yaw * yaw;
        if (mouse_pitch < abs_pitch && pitch != 0.0f)
            pitch *= mouse_pitch / abs_pitch;
        aim_assist_mouse_movement_since_last_tick.yaw = 0.0f;
        aim_assist_mouse_movement_since_last_tick.pitch = 0.0f;
        if (cut_yaw)
            yaw = cut_yaw_value;
    }
    apply_silent_rotation(Rot{yaw, pitch});
}

// Every frame: turn towards the aim point (or, without a target, back to the real view).
extern "C" void aim_assist_frame(FrameEvt *h) {
    Ctx *ctx = h->ctx;
    if (ctx->in_game != 1)
        return;

    if (!aim_assist_has_target) {
        if (aim_assist_aiming != 1)
            return;
        // No target any more: hand the rotation back by turning to where the player looks.
        if (!is_silent_rotation_active() || !update_rotation_priority_peak(100)) {
            aim_assist_aiming = 0;
            return;
        }
        Rot current = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
        Rot back = wrap(rot_sub(get_render_rotation(), current));
        if (10.0f > rot_length(back))
            aim_assist_aiming = 0;   // close enough: done after this frame
        aim_assist_apply_frame_rotation(back, (float)h->dt);
        return;
    }

    if (!update_rotation_priority_peak(100))
        return;
    if (!aim_assist_aiming && aim_assist_mode == 3)
        aim_assist_aiming = 1;
    if (aim_assist_aiming && !is_silent_rotation_active())
        return;

    // Our eye position this frame, and the aim point between last tick's and this tick's.
    float partial = h->partial;
    Vec3 prev = java_get_entity_prev_pos_x(ctx->jvm, ctx->player);
    Vec3 pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Vec3 eye = vec3_add(prev, vec3_scale(vec3_sub(pos, prev), partial));
    if (!java_variant(ctx->jvm))
        eye = vec3_add_eye_height(eye);
    Vec3 aim = vec3_lerp(aim_assist_previous_current_aim_point, aim_assist_previous_current_aim_point_second, h->partial);

    Rot want = rot_look_at(eye, aim);
    Rot current = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
    Rot diff = wrap(rot_sub(want, current));
    if (aim_assist_mode == 3 && !aim_assist_silent_ignore_manual_aim) {
        // Silent: if the player is aiming at it themselves (closer than we are), follow them.
        Rot back = wrap(rot_sub(get_render_rotation(), current));
        Rot from_player = wrap(rot_sub(want, get_render_rotation()));
        if (10.0f > rot_length(back) && rot_length(diff) > rot_length(from_player)) {
            apply_silent_rotation(back);
            return;
        }
    }
    aim_assist_apply_frame_rotation(diff, (float)h->dt);
}

// Forget the target.
static void clear_target(Ctx *ctx) {
    if (aim_assist_cached_target_ref)
        delete_global_ref(ctx->jvm, aim_assist_cached_target_ref);
    aim_assist_cached_target_ref = nullptr;
    aim_assist_has_target = 0;
}

// Should we aim at all this tick? Clears or pauses the target when not.
static bool conditions_ok(Ctx *ctx) {
    if (ctx->in_game != 1) {
        clear_target(ctx);
        return false;
    }
    if (aim_assist_require_sprint == 1 && aim_assist_mode != 3 && !java_call_entity_is_sprinting(ctx->jvm, ctx->player)) {
        aim_assist_has_target = 0;   // pause, keep the target
        return false;
    }
    unsigned char mouse_pressed = vertex_pointer_current(ctx);
    if (aim_assist_require_mouse_pressed == 1) {
        if (!mouse_pressed) {
            // Start aiming only after 1-2 clicks next time.
            aim_assist_clicks_wait_before_aiming = random_chance(0.5f) + 1;
            clear_target(ctx);
            return false;
        }
        if ((unsigned)aim_assist_mode < 2 && aim_assist_clicks_wait_before_aiming) {
            aim_assist_clicks_wait_before_aiming -= 1;
            clear_target(ctx);
            return false;
        }
    }
    if ((mouse_pressed & aim_assist_disable_on_block_break) == 1) {
        void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        if (hit && java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 1) {
            aim_assist_has_target = 0;   // mining a block: pause
            return false;
        }
    }
    if (aim_assist_require_weapon == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player)) {
        clear_target(ctx);
        return false;
    }
    return true;
}

// Every tick: choose (or keep) the target and its aim point.
extern "C" void aim_assist_tick(Hook *hook) {
    aim_assist_mouse_movement_since_last_tick = Rot{0.0f, 0.0f};
    Ctx *ctx = hook->ctx;
    if (!conditions_ok(ctx))
        return;

    Vec3 eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (!java_variant(ctx->jvm))
        eye = vec3_add_eye_height(eye);
    Rot view = is_silent_rotation_active() ? get_render_rotation() : get_entity_rotation(ctx->jvm, ctx->player);
    Vec3 aim;

    // Keep the current target while it stays valid ("mode_target" 0).
    if (!aim_assist_mode_target && aim_assist_cached_target_ref) {
        double range = aim_assist_range;
        float fov = aim_assist_fov, best_hp = BIGGEST_FLOAT;
        int best_hurt = 0x7fffffff;
        unsigned best_tie = 0;
        if (aim_assist_test_entity_updating(ctx, aim_assist_cached_target_ref, &eye, view, 1, &range, &best_hp, &best_hurt, &best_tie, &fov, &aim, &aim_assist_has_target)) {
            aim_assist_previous_current_aim_point = aim_assist_previous_current_aim_point_second;
            aim_assist_previous_current_aim_point_second = aim;
            return;
        }
        delete_global_ref(ctx->jvm, aim_assist_cached_target_ref);
        aim_assist_cached_target_ref = nullptr;
        aim_assist_has_target = 0;
    }

    // Find the best player.
    double range = aim_assist_range;
    float fov = aim_assist_fov, best_hp = BIGGEST_FLOAT;
    int best_hurt = 0x7fffffff;
    unsigned best_tie = 0;
    unsigned char should_aim;
    void *best = nullptr;
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        if ((*env)->IsSameObject(env, ctx->player, other))
            continue;   // ourselves
        if (aim_assist_test_entity_updating(ctx, other, &eye, view, 0, &range, &best_hp, &best_hurt, &best_tie, &fov, &aim, &should_aim))
            best = other;
    }
    if (!best) {
        clear_target(ctx);
        return;
    }

    // Same target as before: move the aim point on; a new one: jump to its point.
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    if ((*env)->IsSameObject(env, aim_assist_cached_target_ref, best)) {
        aim_assist_previous_current_aim_point = aim_assist_previous_current_aim_point_second;
        aim_assist_previous_current_aim_point_second = aim;
    } else {
        aim_assist_previous_current_aim_point_second = aim;
        aim_assist_previous_current_aim_point = aim;
    }
    aim_assist_has_target = should_aim;
    if (aim_assist_cached_target_ref)
        (*env)->DeleteGlobalRef(env, aim_assist_cached_target_ref);
    if (void *ref = (*env)->NewGlobalRef(env, best))
        aim_assist_cached_target_ref = ref;
}
