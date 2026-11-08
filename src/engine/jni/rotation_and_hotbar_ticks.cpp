// 0x2a000: rotation save/restore, hotbar slot switching, tagged-float entity flags, small id sets
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"

struct F2 { float a, b; };
FN(void *, get_minecraft, (void *jvm));
FN(void *, get_local_player, (void *jvm, void *minecraft));
FN(unsigned long long, get_entity_rotation, (void *jvm, void *entity));               // packed Rot
FN(void, set_entity_rotation, (void *jvm, void *entity, unsigned long long rot));
FN(void, set_entity_prev_rotation, (void *jvm, void *entity, unsigned long long rot));
FN(float, java_get_entity_player_speed_on_ground, (void *jvm, void *entity_player));                           // tagged flag float
FN(void, java_set_entity_player_speed_on_ground, (void *jvm, void *entity_player, float value));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inv));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inv));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void, java_set_inventory_player_current_item, (void *jvm, void *inv, int slot));
FN(unsigned long long, libc_strlen, (const char *str));                // strlen
FN(void **, element_at, (void *vec, int elem, void *hdr, void *key));
FN(int, key_name_to_code, (void *key));
FN(void *, key_code_to_index, (int idx));
FN(void *, vector_find_insert, (void *t, int elem, void *hdr, void *e, void *cmp));
FN(void *, vector_erase, (void *vec, int elem, void *hdr, void *key));
FN(void *, binary_search, (void *vec, int elem, void *hdr, void *e, void *cmp));

extern "C" unsigned long long silent_rotation_current, rotation_hotbar_saved_current_rotation;   // saved / current rotation
extern "C" unsigned char silent_rotation_active_flag, render_rotation_dirty, rotation_hotbar_swap_pending;
extern "C" void *rotation_hotbar_slot_owner;                             // slot owner
extern "C" void *rotation_hotbar_module_that_switched_slot;                             // module that switched the slot
extern "C" int rotation_hotbar_item_id, refill_state_ticks_since_swap;
extern "C" unsigned char rotation_hotbar_item_pending;

// apply the saved rotation to the local player (silent rotations)
extern "C" void tick_rotation(void *jvm) {
    void *minecraft = get_minecraft(jvm);
    void *player = get_local_player(jvm, minecraft);
    if (!player) return;
    rotation_hotbar_saved_current_rotation = get_entity_rotation(jvm, player);
    if (render_rotation_dirty == 1) {
        render_rotation_dirty = 0;
        set_entity_rotation(jvm, player, silent_rotation_current);
        set_entity_prev_rotation(jvm, player, silent_rotation_current);
    }
    if (silent_rotation_active_flag == 1) {
        rotation_hotbar_swap_pending = 1;
        set_entity_rotation(jvm, player, silent_rotation_current);
    }
}
// restore the real rotation
extern "C" void apply_rotation_to_player(void *jvm, void *object) {
    if (rotation_hotbar_swap_pending != 1) return;
    silent_rotation_current = get_entity_rotation(jvm, object);
    rotation_hotbar_swap_pending = 0;
    set_entity_rotation(jvm, object, rotation_hotbar_saved_current_rotation);
    set_entity_prev_rotation(jvm, object, rotation_hotbar_saved_current_rotation);
}

struct GRef { void *padding_00; jobject ref; };
extern "C" void tick_post_frame(Jvm **jj, GRef *g) {
    if (g->ref) JNI(*jj)->DeleteGlobalRef((*jj)->env, g->ref);
    g->ref = nullptr;
}
// claim / release the hotbar for a module
extern "C" bool claim_release_hotbar_module(void *owner) {
    void *cur = rotation_hotbar_slot_owner;
    if (cur == owner) return true;
    if (cur) return false;
    rotation_hotbar_slot_owner = owner;
    return true;
}
extern "C" void release_owner(void *owner) {
    if (rotation_hotbar_slot_owner == owner) rotation_hotbar_slot_owner = nullptr;
}
// slot index -> (column, row) of the inventory grid; hotbar slots get row 1
extern "C" F2 slot_index(unsigned slot) {
    F2 r;
    if (slot <= 8) {
        r.b = 1.0f;
        r.a = (float)((int)slot - 5);
    } else {
        unsigned q = slot / 9;
        r.b = (float)(int)(slot - q * 9);
        r.a = (float)(q + 3);
    }
    return r;
}
// append a letter/digit/space to a 100-byte text buffer
extern "C" void key_name_append(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    if (n > 99) return;
    if ((unsigned char)((character & 0xdf) + 0xbf) >= 0x1a && (unsigned char)(character - '0') >= 10 && character != ' ') return;
    buf[n] = character;
}

struct IdVec { void *v; char hdr[4]; int count; };
struct IdEnt { void *p; int id; int z; };
extern "C" int id_vector_count(IdVec *id_vector) { return id_vector->count; }
extern "C" void *id_vector_get(IdVec *id_vector, void *key) { return *element_at(id_vector->v, 0x10, id_vector->hdr, key); }
extern "C" int compare_id_entry(void *left, IdEnt *right, IdEnt *b) { return right->id - b->id; }
extern "C" bool id_vector_insert(IdVec *id_vector, void *key) {
    int id = key_name_to_code(key);
    if (id != -1) {
        IdEnt e = {key_code_to_index(id), id, 0};
        vector_find_insert(id_vector, 0x10, id_vector->hdr, &e, (void *)compare_id_entry);
    }
    return id != -1;
}
extern "C" void *id_vector_remove(IdVec *id_vector, void *key) { return vector_erase(id_vector->v, 0x10, id_vector->hdr, key); }
extern "C" void id_vector_clear(IdVec *id_vector) { id_vector->count = 0; }
extern "C" bool id_vector_contains(IdVec *id_vector, int id) {
    IdEnt e = {nullptr, id, 0};
    return binary_search(id_vector->v, 0x10, id_vector->hdr, &e, (void *)compare_id_entry) != nullptr;
}

// entity flags kept in a float field as a NaN tagged 0xb315xxxx
#define TAG 0xb3150000u
static inline __attribute__((always_inline)) unsigned bits(float f) { return __builtin_bit_cast(unsigned, f); }
extern "C" unsigned entity_flags_get(void *jvm, void *entity) {
    unsigned v = bits(java_get_entity_player_speed_on_ground(jvm, entity));
    return (v & 0xffff0000) == TAG ? (v & 0xffff) : 0;
}
extern "C" bool entity_flags_has(void *jvm, void *entity, int mask) {
    unsigned v = bits(java_get_entity_player_speed_on_ground(jvm, entity));
    unsigned f = (v & 0xffff0000) == TAG ? (v & 0xffff) : 0;
    return (f & mask) != 0;
}
extern "C" void entity_flags_set(void *jvm, void *entity, int mask) {
    unsigned v = bits(java_get_entity_player_speed_on_ground(jvm, entity));
    unsigned cur = (v & 0xffff0000) == TAG ? v : TAG;
    java_set_entity_player_speed_on_ground(jvm, entity, __builtin_bit_cast(float, ((cur | mask) & 0xffff) | TAG));
}
extern "C" void entity_flags_clear(void *jvm, void *entity, int mask) {
    unsigned v = bits(java_get_entity_player_speed_on_ground(jvm, entity));
    unsigned cur = (v & 0xffff0000) == TAG ? (v & 0xffff) : TAG;
    java_set_entity_player_speed_on_ground(jvm, entity, __builtin_bit_cast(float, (~mask & cur) | TAG));
}

// switch to the best hotbar slot (exact item id first, else best score), once per module
extern "C" void switch_best_hotbar_slot(Ctx *ctx, void *owner, unsigned char force, int (*score)(int)) {
    if (rotation_hotbar_module_that_switched_slot != nullptr) return;
    if (!force && rotation_hotbar_item_id == 0) return;
    void *inv = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    int want = rotation_hotbar_item_id;
    int start = java_get_inventory_player_current_item(ctx->jvm, inv) + rotation_hotbar_item_pending;
    void *items = java_get_inventory_player_main_inventory(ctx->jvm, inv);
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    int best = -1, bestScore = -1;
    #pragma clang loop unroll(disable)
    for (int i = 0; i != 9; i++) {
        int slot = (start + i) % 9;
        jobject st = (*env)->GetObjectArrayElement(env, (jobjectArray)items, slot);
        if (!st) continue;
        void *item = java_call_item_stack_get_item(ctx->jvm, st);
        if (!item) continue;
        int item_id = java_call_static_item_get_id_from_item(ctx->jvm, item);
        if (want != 0 && item_id == want) { best = slot; break; }
        int s = score(item_id);
        if (s > bestScore) { bestScore = s; best = slot; }
    }
    if (best == -1) return;
    if (!claim_release_hotbar_module(owner)) return;
    java_set_inventory_player_current_item(ctx->jvm, inv, best);
    rotation_hotbar_module_that_switched_slot = owner;
}
// block-id preference for scaffolding: 2 = solid building block, 1 = acceptable, -1 = no
extern "C" int block_id_preference_scaffolding(int id) {
    switch (id) {
    case 1: case 2: case 3: case 35: case 45: return 2;
    case 5: case 17: case 20: case 24: case 87: case 121: case 159: case 162: return 1;
    default: return -1;
    }
}
extern "C" void track_owner(Ctx *ctx, void *owner, unsigned char force) {
    switch_best_hotbar_slot(ctx, owner, force, block_id_preference_scaffolding);
}
extern "C" void release_tracked_owner() {
    if (rotation_hotbar_module_that_switched_slot) {
        release_owner(rotation_hotbar_module_that_switched_slot);
        rotation_hotbar_module_that_switched_slot = nullptr;
    }
    refill_state_ticks_since_swap++;
    rotation_hotbar_item_id = 0;
    rotation_hotbar_item_pending = 0;
}

// ---- "moving toward the target?" test, computed once per tick (state bit 0) ----
struct V3a { double x, y, z; };
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));
FN(void *, java_get_moving_object_position_hit_vec, (void *jvm, void *moving_object_position));
FN(V3a, java_get_vec3_x_coord, (void *jvm, void *vec3));
FN(V3a, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_variant, (void *jvm));
FN(V3a, vec3_add_eye_height, (V3a v));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(float, java_get_entity_living_base_land_movement_factor, (void *jvm, void *entity_living_base));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *object));
FN(float, java_get_entity_living_base_move_forward, (void *jvm, void *entity_living_base));
FN(float, java_get_entity_living_base_move_strafing, (void *jvm, void *entity_living_base));
FN(V3a, horizontal_motion, (unsigned long long rot, float fwd, float strafe, float speed));
FN(V3a, java_get_entity_motion_x, (void *jvm, void *entity));
FN(V3a, vec3_add, (V3a a, V3a b));
FN(double, vec3_distance_sq, (V3a a, V3a b));
extern "C" const float airborne_movement_speed;
extern "C" const double attack_motion_slowdown_factor;
struct TickState { unsigned char done; char padding_01[0x17]; unsigned char result; };
extern "C" void tick_pre_click(Ctx *ctx, TickState *st) {
    if (st->done & 1) return;
    unsigned char r = 1;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (hit && java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 2) {
        V3a vec3_x_coord = java_get_vec3_x_coord(ctx->jvm, java_get_moving_object_position_hit_vec(ctx->jvm, hit));
        V3a eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
        if (!java_variant(ctx->jvm)) eye = vec3_add_eye_height(eye);
        float speed = java_get_entity_on_ground(ctx->jvm, ctx->player) ? java_get_entity_living_base_land_movement_factor(ctx->jvm, ctx->player) : airborne_movement_speed;
        void *jvm = ctx->jvm;
        void *inv = get_game_settings(jvm, ctx->minecraft);
        void *game_settings_key_bindings = java_get_game_settings_forward_key(jvm, inv), *key_binding = java_get_game_settings_back_key(jvm, inv), *i2 = java_get_game_settings_left_key(jvm, inv), *i3 = java_get_game_settings_right_key(jvm, inv);
        is_key_down(jvm, game_settings_key_bindings); is_key_down(jvm, key_binding); is_key_down(jvm, i2); is_key_down(jvm, i3);
        unsigned long long rot = get_entity_rotation(ctx->jvm, ctx->player);
        float fwd = java_get_entity_living_base_move_forward(ctx->jvm, ctx->player);
        V3a mv = horizontal_motion(rot, fwd, java_get_entity_living_base_move_strafing(ctx->jvm, ctx->player), speed);
        V3a motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
        V3a entity_motion_x = java_get_entity_motion_x(ctx->jvm, ctx->player);
        entity_motion_x.x *= attack_motion_slowdown_factor;
        entity_motion_x.z *= attack_motion_slowdown_factor;
        V3a da = vec3_add(motion, mv);
        V3a db = vec3_add(entity_motion_x, mv);
        V3a pa = vec3_add(eye, da);
        V3a pb = vec3_add(eye, db);
        double dB = vec3_distance_sq(pb, vec3_x_coord);
        r = vec3_distance_sq(pa, vec3_x_coord) > dB;
    }
    st->result = r;
    st->done |= 1;
}

// ---- pick the aim target: crosshair entity, forced target, else nearest valid entity in FOV ----
struct AB { V3a min, max; };
struct Rot2 { float yaw, pitch; };
FN(void *, java_get_moving_object_position_entity_hit, (void *jvm, void *moving_object_position));
FN(void *, java_jni_is_instance_of_entity_other_player_mp, (void *jvm, void *candidate));
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(void *, get_refill_state, ());
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(unsigned char, java_get_entity_player_sleeping, (void *jvm, void *entity_player));
FN(void *, java_get_entity_bounding_box, (void *jvm, void *entity));
FN(AB, java_get_axis_aligned_bb_min_x, (void *jvm, void *bounding_box));
FN(AB, aabb_expand, (AB b, double d));
FN(V3a, aabb_clamp_point, (AB b, V3a p));
FN(Rot2, rot_look_at, (V3a from, V3a to));
FN(float, wrap_angle, (float x));
extern "C" const double tenth_in_rotation_and_hotbar_ticks, search_range_squared;
extern "C" const float threshold_4900;
struct Target { int z0; int padding_04; jobject ref; double dist; };
extern "C" bool target_clear(Ctx *ctx, Target *target) {
    target->z0 = 0;
    if (!ctx->world) {
        if (target->ref) { JNIEnv *env = ((Jvm *)ctx->jvm)->env; (*env)->DeleteGlobalRef(env, target->ref); }
        target->ref = 0;
        return true;
    }
    V3a eye = java_get_entity_pos_x(ctx->jvm, ctx->player);
    if (!java_variant(ctx->jvm)) eye = vec3_add_eye_height(eye);
    void *ent;
    JNIEnv *env;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (hit && java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 2 && (ent = java_jni_is_instance_of_entity_other_player_mp(ctx->jvm, java_get_moving_object_position_entity_hit(ctx->jvm, hit))) != 0 &&
        !entity_flags_has(ctx->jvm, ent, 7) && java_call_entity_living_base_get_health(ctx->jvm, ent) > 0.0f) {
        env = ((Jvm *)ctx->jvm)->env;
    } else {
        ent = get_refill_state();
        env = ((Jvm *)ctx->jvm)->env;
        if (!ent) {
            Rot2 rot = __builtin_bit_cast(Rot2, get_entity_rotation(ctx->jvm, ctx->player));
            void *list = java_get_world_player_entities(ctx->jvm, ctx->world);
            void *best = 0;
            double bestD = search_range_squared;
            for (int i = 0; i < java_call_list_size(ctx->jvm, list); i++) {
                void *element = java_call_list_get(ctx->jvm, list, i);
                JNIEnv *e2 = ((Jvm *)ctx->jvm)->env;
                if ((*e2)->IsSameObject(e2, (jobject)ctx->player, (jobject)element)) continue;
                if (entity_flags_has(ctx->jvm, element, 7)) continue;
                if (!(java_call_entity_living_base_get_health(ctx->jvm, element) > 0.0f)) continue;
                if (java_get_entity_player_sleeping(ctx->jvm, element)) continue;
                AB bb = aabb_expand(java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, element)), tenth_in_rotation_and_hotbar_ticks);
                double d = vec3_distance_sq(eye, aabb_clamp_point(bb, eye));
                if (d > bestD) continue;
                Rot2 ang = rot_look_at(eye, java_get_entity_pos_x(ctx->jvm, element));
                float dp = ang.pitch - rot.pitch;
                float dy = wrap_angle(ang.yaw - rot.yaw);
                if (dy * dy + dp * dp > threshold_4900) continue;
                bestD = d;
                best = element;
            }
            if (best) target->dist = bestD;
            env = ((Jvm *)ctx->jvm)->env;
            if (target->ref) (*env)->DeleteGlobalRef(env, target->ref);
            if (!best) {
                target->ref = 0;
                return true;
            }
            jobject r = (*env)->NewGlobalRef(env, (jobject)best);
            if (!r) return false;
            target->ref = r;
            return true;
        }
    }
    if (target->ref) (*env)->DeleteGlobalRef(env, target->ref);
    jobject r = (*env)->NewGlobalRef(env, (jobject)ent);
    if (r) target->ref = r;
    AB bb = aabb_expand(java_get_axis_aligned_bb_min_x(ctx->jvm, java_get_entity_bounding_box(ctx->jvm, ent)), tenth_in_rotation_and_hotbar_ticks);
    target->dist = vec3_distance_sq(eye, aabb_clamp_point(bb, eye));
    return true;
}