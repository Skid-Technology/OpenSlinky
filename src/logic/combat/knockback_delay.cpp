// Knockback Delay - take knockback later than you should.
//
// When the server sends you knockback, this module holds that packet (and everything that
// arrives after it) for up to "max_delay" ms before the game sees it. That keeps you in
// place a little longer during a fight. Only when the conditions allow: in the air, holding
// a weapon, a target in range, ..., and "chance".
//
// Callbacks (descriptor slots +0x68 / +0x168):
//   knockback_delay_on_packet_received  every incoming packet (network thread): hold or release
//   knockback_delay_on_late_tick        every tick: decide whether delaying is allowed
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
struct Range { float lo, hi; };

struct PktHook {
    char padding_00[8];
    void *ctx;              // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    long long budget;       // +0x18 ms the packet has been held so far
    int id;                 // +0x20 id of the batch of packets it arrived in
    int verdict;            // +0x24 0 = let through, 1 = hold, 3 = hold (and it is a hurt/health/sound packet)
};

// ---- settings ----
extern "C" float knockback_delay_max_delay;                 // "max_delay" (ms)
extern "C" unsigned char knockback_delay_require_air_value; // "require_air"
extern "C" float knockback_delay_range_value;               // "range": target must be this close
extern "C" unsigned char knockback_delay_require_weapon_value;   // "require_weapon"
extern "C" unsigned char knockback_delay_require_mouse_value;    // "require_mouse"
extern "C" unsigned char knockback_delay_require_target_value;   // "require_target": a player within 6 blocks in front of us
extern "C" float knockback_delay_chance_value;              // "chance"

// ---- module state (shared with the network thread) ----
extern "C" unsigned char knockback_delay_holding;           // packets are being held
extern "C" int knockback_delay_held_transaction_id;         // batch id of the held knockback packet
extern "C" unsigned char knockback_delay_air;               // "require_air" is on and we are in the air
extern "C" unsigned char knockback_delay_allowed;           // conditions say delaying is allowed
extern "C" unsigned short knockback_delay_max_delay_second; // "max_delay" as a whole number
extern "C" int knockback_delay_own_entity_id;
extern "C" char knockback_delay_random_walk_1[];            // random walk state

// ---- game / engine functions ----
FN(void *, java_jni_is_instance_of_s12_packet_entity_velocity, (void *ctx, void *event));   // knockback packet, or null
FN(int, java_get_s12_packet_entity_velocity_entity_id, (void *ctx, void *pkt));
FN(void *, java_jni_is_instance_of_s19_packet_entity_status, (void *ctx, void *event));
FN(void *, java_jni_is_instance_of_s06_packet_update_health, (void *ctx, void *event));
FN(void *, java_jni_is_instance_of_s29_packet_sound_effect, (void *ctx, void *event));
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                       // a mouse button is held
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(Rot, rot_look_at, (Vec3 from, Vec3 to));
FN(float, wrap_angle, (float a));                                                           // to -180..180
FN(Range, rot_make_floats, (float lo, float hi));
FN(unsigned char, walk_should_change, (void *state, Range r, double p));                   // random yes/no

// Stop holding and forget the held packet.
static void stop_holding() {
    knockback_delay_holding = 0;
    __atomic_exchange_n(&knockback_delay_allowed, (unsigned char)0, __ATOMIC_SEQ_CST);
    knockback_delay_held_transaction_id = 0;
}

// Every incoming packet (network thread).
extern "C" void knockback_delay_on_packet_received(PktHook *hook) {
    if (hook->verdict != 0)
        return;

    // Is it knockback for us?
    bool our_knockback = false;
    void *velocity = java_jni_is_instance_of_s12_packet_entity_velocity(hook->ctx, hook->ev);
    if (velocity)
        our_knockback = java_get_s12_packet_entity_velocity_entity_id(hook->ctx, velocity) == __atomic_load_n(&knockback_delay_own_entity_id, __ATOMIC_SEQ_CST);

    // Should this packet be held?
    unsigned char hold;
    if (__atomic_load_n(&knockback_delay_air, __ATOMIC_SEQ_CST) & 1) {
        // In the air: start holding with the first packet of a batch.
        hold = hook->budget == 0 ? (__atomic_load_n(&knockback_delay_allowed, __ATOMIC_SEQ_CST) & 1) : 0;
        knockback_delay_holding = hold;
    } else if ((__atomic_load_n(&knockback_delay_allowed, __ATOMIC_SEQ_CST) & our_knockback) & 1) {
        // Our knockback arrived: start holding.
        knockback_delay_holding = 1;
        hold = 1;
    } else {
        hold = knockback_delay_holding;
    }

    if ((our_knockback & hold) == 1) {
        // Remember which batch the knockback came in. A second knockback in another batch: give up.
        int held_id = knockback_delay_held_transaction_id;
        int id = hook->id;
        if (held_id == 0) {
            knockback_delay_held_transaction_id = id;
        } else if (held_id != id) {
            stop_holding();
            return;
        }
    } else if (!hold) {
        return;
    }

    // Held long enough: let everything through.
    if (hook->budget > (long long)__atomic_load_n(&knockback_delay_max_delay_second, __ATOMIC_SEQ_CST)) {
        stop_holding();
        return;
    }
    bool hurt_related = java_jni_is_instance_of_s19_packet_entity_status(hook->ctx, hook->ev) ||
                        java_jni_is_instance_of_s06_packet_update_health(hook->ctx, hook->ev) ||
                        java_jni_is_instance_of_s29_packet_sound_effect(hook->ctx, hook->ev);
    hook->verdict = hurt_related ? 3 : 1;
}

// Is another player within 6 blocks and within 90 degrees of where we look?
static bool player_in_front(Ctx *ctx) {
    Vec3 my_pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Rot rotation = get_entity_rotation(ctx->jvm, ctx->player);
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        if ((*env)->IsSameObject(env, ctx->player, other))
            continue;   // ourselves
        Vec3 other_pos = java_get_entity_pos_x(ctx->jvm, other);
        if (vec3_distance_sq(my_pos, other_pos) > 36.0)
            continue;
        float angle = __builtin_fabsf(wrap_angle(rot_look_at(my_pos, other_pos).yaw - rotation.yaw));
        if (angle > 90.0f)
            continue;
        return true;
    }
    return false;
}

// Target in range, weapon, mouse and "require_target" conditions.
static bool conditions_met(Ctx *ctx) {
    CombatTarget *target = (CombatTarget *)ctx->target;
    if (!target->ent || target->dist2 > (double)(knockback_delay_range_value * knockback_delay_range_value))
        return false;
    if (knockback_delay_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return false;
    if (knockback_delay_require_mouse_value == 1 && !vertex_pointer_current(ctx))
        return false;
    if (knockback_delay_require_target_value == 1 && !player_in_front(ctx))
        return false;
    return true;
}

// "chance": always yes above 0.999, otherwise a random walk around the chance.
static unsigned char roll_chance() {
    double chance = knockback_delay_chance_value;
    if (chance > 0.999)
        return 1;
    return walk_should_change(knockback_delay_random_walk_1, rot_make_floats((float)(-0.025 + chance), (float)(chance + 0.025)), 60.0);
}

// Every tick: is delaying allowed right now?
extern "C" void knockback_delay_on_late_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (!ctx->player)
        return;
    __atomic_exchange_n(&knockback_delay_own_entity_id, java_get_entity_entity_id(ctx->jvm, ctx->player), __ATOMIC_SEQ_CST);

    unsigned char in_air;
    if (knockback_delay_require_air_value == 1)
        in_air = !java_get_entity_on_ground(ctx->jvm, ctx->player);
    else
        in_air = 0;
    __atomic_exchange_n(&knockback_delay_air, in_air, __ATOMIC_SEQ_CST);

    unsigned char allowed = 0;
    if (knockback_delay_require_air_value == 1 && !in_air)
        knockback_delay_holding = 0;
    else if (conditions_met(ctx))
        allowed = roll_chance();

    __atomic_exchange_n(&knockback_delay_allowed, allowed, __ATOMIC_SEQ_CST);
    __atomic_exchange_n(&knockback_delay_max_delay_second, (unsigned short)(int)knockback_delay_max_delay, __ATOMIC_SEQ_CST);
}
