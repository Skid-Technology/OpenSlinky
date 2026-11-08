// Criticals - land critical hits more often.
//
// A hit is critical when you are falling. Two ways to get there ("mode"):
//   0 "packet": hold your movement packets back for up to "max_delay" ms while you fall,
//               so the server still sees you falling when the hit arrives.
//   1 "timer":  slow the game down while you fall onto the player under your crosshair,
//               then speed it up again afterwards ("boost") to catch up.
// Each 10 ticks, "chance" decides whether the module helps for the next 10 ticks.
//
// Callbacks (descriptor slots +0x48 / +0x58 / +0x78 / +0x98):
//   criticals_on_tick         every tick: decide whether to act
//   criticals_on_frame        every frame: change the game speed (timer mode)
//   criticals_on_packet_sent  every movement packet: hold it back (packet mode)
//   criticals_on_toggle       module switched off: restore the game speed
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };

struct PktHook {
    char padding_00[8];
    void *ctx;              // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    long long budget;       // +0x18 ms the packet has been held so far
    int seq;                // +0x20 packet sequence number
    int verdict;            // +0x24 set to 1 to keep holding it
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct TimerHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    char padding_10[8];
    double dt;           // +0x18 seconds since the last frame
};

// ---- settings ----
enum CriticalsMode { MODE_PACKET = 0, MODE_TIMER = 1 };
extern "C" unsigned criticals_mode;                            // "mode"
extern "C" float criticals_max_delay_value;                    // "max_delay" (ms, packet mode)
extern "C" float criticals_chance_value;                       // "chance"
extern "C" unsigned char criticals_require_mouse_pressed_value;   // "require_mouse_pressed"
extern "C" unsigned char criticals_require_weapon_value;       // "require_weapon"
extern "C" float criticals_timer_speed_value;                  // "timer_speed": game speed while falling (timer mode)
extern "C" float criticals_boost_value;                        // "boost": max ticks to catch up afterwards

// ---- module state ----
extern "C" char criticals_module_descriptor[];
// Shared with the network thread, so read/written atomically.
extern "C" unsigned char criticals_packet_mode_armed;   // packet mode is active
extern "C" unsigned short criticals_packet_delay;       // max hold time (ms)
extern "C" unsigned char criticals_timer_mode_armed;    // timer mode is slowing the game this tick
extern "C" int criticals_roll_cooldown;                 // ticks until the next "chance" roll
extern "C" unsigned char criticals_last_roll;           // result of the last roll
extern "C" unsigned char criticals_timer_modified;      // we changed the game speed
extern "C" double criticals_accumulated_boost;          // ticks to catch up on
extern "C" int criticals_current_hold_budget;           // how long packets may be held (ms)
extern "C" double criticals_last_y;                     // y in the last movement packet
extern "C" int criticals_last_sequence;                 // sequence number of the last packet seen
extern "C" unsigned char criticals_falling;             // the last packets show us falling

// ---- game / engine functions ----
FN(unsigned char, random_chance, (float chance));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                       // a mouse button is held
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 2 = an entity
FN(void *, get_timer, (void *jvm, void *timer));                                             // the game's tick timer
FN(void, java_set_timer_timer_speed, (void *jvm, void *timer, float speed));
FN(float, java_get_timer_elapsed_partial_ticks, (void *jvm, void *timer));
FN(void, java_set_timer_elapsed_partial_ticks, (void *jvm, void *timer, float value));
FN(void *, java_jni_is_instance_of_c03_packet_player, (void *ctx, void *event));          // movement packet, or null
FN(unsigned char, java_get_c03_packet_player_moving, (void *ctx, void *c03_packet_player));   // contains a position
FN(double, java_get_c03_packet_player_b, (void *ctx, void *c03_packet_player));            // its y
FN(unsigned char, java_get_c03_packet_player_on_ground, (void *ctx, void *c03_packet_player));

// Every tick: should the module help right now?
static bool should_act(Ctx *ctx) {
    if (ctx->in_game != 1)
        return false;

    // Roll "chance" once every 10 ticks.
    int cooldown = criticals_roll_cooldown;
    if (cooldown != 0) {
        unsigned char last_roll = criticals_last_roll;
        criticals_roll_cooldown = cooldown - 1;
        if (last_roll != 1)
            return false;
    } else {
        criticals_roll_cooldown = 10;
        unsigned char roll = random_chance(criticals_chance_value);
        criticals_last_roll = roll;
        criticals_roll_cooldown += -1;
        if (!roll)
            return false;
    }

    if (criticals_require_mouse_pressed_value == 1 && !vertex_pointer_current(ctx))
        return false;
    if (criticals_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return false;
    // Only while fighting: we attacked in the last 10 ticks.
    int last_attack = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
    return java_get_entity_ticks_existed(ctx->jvm, ctx->player) - last_attack < 11;
}

extern "C" void criticals_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    bool act = should_act(ctx);

    if (criticals_mode == MODE_PACKET) {
        __atomic_exchange_n(&criticals_packet_mode_armed, (unsigned char)act, __ATOMIC_SEQ_CST);
        __atomic_exchange_n(&criticals_packet_delay, (unsigned short)(int)criticals_max_delay_value, __ATOMIC_SEQ_CST);
        criticals_timer_mode_armed = 0;
        return;
    }
    if (criticals_mode != MODE_TIMER)
        return;
    __atomic_exchange_n(&criticals_packet_mode_armed, (unsigned char)0, __ATOMIC_SEQ_CST);

    // Timer mode: slow down while falling (not just hit) onto the entity under the crosshair.
    unsigned char slow_down = 0;
    if (act && !java_get_entity_on_ground(ctx->jvm, ctx->player) &&
        java_get_entity_motion_x(ctx->jvm, ctx->player).y < 0.0 &&
        java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player) != 10) {
        void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        if (hit)
            slow_down = java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 2;
    }
    criticals_timer_mode_armed = slow_down;
}

// Every frame (timer mode).
extern "C" void criticals_on_frame(TimerHook *hook) {
    Ctx *ctx = hook->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);

    if (criticals_timer_mode_armed == 1) {
        // Slow the game down, and remember how many ticks we lose (up to "boost").
        java_set_timer_timer_speed(ctx->jvm, timer, criticals_timer_speed_value);
        criticals_timer_modified = 1;
        double ticks_this_frame = hook->dt * 20.0;
        double lost = (double)(1.0f - criticals_timer_speed_value) * ticks_this_frame + criticals_accumulated_boost;
        criticals_accumulated_boost = lost;
        if (lost > (double)criticals_boost_value)
            criticals_accumulated_boost = criticals_boost_value;
        return;
    }

    // Not slowing down: catch up on the lost ticks, whole ticks at a time.
    double lost = criticals_accumulated_boost;
    if (lost > 1.0) {
        float partial = java_get_timer_elapsed_partial_ticks(ctx->jvm, timer);
        int whole_ticks = (int)lost;
        java_set_timer_elapsed_partial_ticks(ctx->jvm, timer, (float)whole_ticks + partial);
        criticals_accumulated_boost -= (double)whole_ticks;
    }
    if (criticals_timer_modified == 1) {
        java_set_timer_timer_speed(ctx->jvm, timer, 1.0f);
        criticals_timer_modified = 0;
    }
}

// Every outgoing packet (packet mode, runs on the network thread).
extern "C" void criticals_on_packet_sent(PktHook *hook) {
    void *packet = java_jni_is_instance_of_c03_packet_player(hook->ctx, hook->ev);
    if (packet && java_get_c03_packet_player_moving(hook->ctx, packet) && hook->seq > criticals_last_sequence) {
        // A new movement packet: are we falling?
        double y = java_get_c03_packet_player_b(hook->ctx, packet);
        unsigned char on_ground = java_get_c03_packet_player_on_ground(hook->ctx, packet);
        if (on_ground || criticals_last_y > y)
            criticals_falling = on_ground ^ 1;

        // While falling, allow holding packets a bit longer each time (1/5 of the max), up to the max.
        int hold = 0;
        if ((__atomic_load_n(&criticals_packet_mode_armed, __ATOMIC_SEQ_CST) & 1) && (criticals_falling & 1)) {
            hold = __atomic_load_n(&criticals_packet_delay, __ATOMIC_SEQ_CST) / 5 + criticals_current_hold_budget;
            if (hold > (int)__atomic_load_n(&criticals_packet_delay, __ATOMIC_SEQ_CST))
                hold = __atomic_load_n(&criticals_packet_delay, __ATOMIC_SEQ_CST);
        }
        criticals_current_hold_budget = hold;
        criticals_last_y = y;
        criticals_last_sequence = hook->seq;
    }
    // Keep holding the packet while it has been held for less than the budget.
    if (hook->budget < (long long)criticals_current_hold_budget)
        hook->verdict = 1;
}

// On disable: restore normal game speed.
extern "C" void criticals_on_toggle(EvHook *hook) {
    if (hook->ev != criticals_module_descriptor || hook->on || !(criticals_timer_modified & 1))
        return;
    Ctx *ctx = hook->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);
    java_set_timer_timer_speed(ctx->jvm, timer, 1.0f);
    criticals_timer_modified = 0;
}
