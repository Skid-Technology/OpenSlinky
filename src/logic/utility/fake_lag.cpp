// Fake Lag - make your connection look laggy on purpose.
//
// Packets are held for up to "inbound" / "outbound" ms before they go through, so others
// see you move in jumps and react later. "require_attack" only lags for 20 ticks after you
// attack. In "mode" 1 (pulse) the lag comes in waves: held, then released every few ticks.
// With "realtime_damage", hit feedback (hurt / health / sound) always arrives at once.
//
// Callbacks (descriptor slots +0x48 / +0x78):
//   fake_lag_on_tick         every tick: decide how long packets may be held
//   fake_lag_on_packet_sent  every packet (both directions): hold or release
#include "slinky/engine.h"

// ---- settings ----
extern "C" float fake_lag_inbound_value;              // "inbound" (ms)
extern "C" float fake_lag_outbound_value;             // "outbound" (ms)
extern "C" bool fake_lag_realtime_damage_value;       // "realtime_damage"
extern "C" unsigned fake_lag_mode;                    // "mode": 0 = constant, 1 = pulse
extern "C" unsigned char fake_lag_require_ingame_value;   // "require_ingame"
extern "C" unsigned char fake_lag_require_weapon_value;   // "require_weapon"
extern "C" unsigned char fake_lag_require_attack_value;   // "require_attack"

// ---- module state ----
extern "C" int fake_lag_last_hurt_time;               // tick of our last attack, last tick
extern "C" int fake_lag_cooldown;                     // ticks of lag left after an attack
extern "C" int fake_lag_inbound_counter;              // pulse mode: ms until the next release
extern "C" int fake_lag_outbound_counter;
// Shared with the network thread:
extern "C" unsigned char fake_lag_realtime_damage_flag;
extern "C" unsigned char fake_lag_inbound_window_open;     // pulse mode: release now
extern "C" unsigned char fake_lag_outbound_window_open;
extern "C" volatile unsigned short fake_lag_inbound_budget;    // ms a packet may be held (0 = no lag)
extern "C" volatile unsigned short fake_lag_outbound_budget;

struct PktHook {
    int type;               // +0x00 2 = incoming
    char padding_04[4];
    Ctx *ctx;               // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    long long budget;       // +0x18 ms the packet has been held so far
    char padding_20[4];
    int verdict;            // +0x24 1 = hold, 3 = let through
};

// ---- game / engine functions ----
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_jni_is_instance_of_s19_packet_entity_status, (void *jvm, void *event));
FN(void *, java_jni_is_instance_of_s06_packet_update_health, (void *jvm, void *event));
FN(void *, java_jni_is_instance_of_s29_packet_sound_effect, (void *jvm, void *event));

// Pulse mode, one direction: count 50 ms per tick down; at the end release and start again.
static bool pulse_step(int *counter, float length) {
    int ms_left = *counter;
    if (ms_left > 0) {
        *counter = ms_left - 50;
        return (unsigned)ms_left < 51;   // the last tick of the wave: release
    }
    *counter = (int)((float)ms_left + length);
    return false;
}

static void set_budgets(unsigned short inbound, unsigned short outbound) {
    __atomic_exchange_n(&fake_lag_inbound_budget, inbound, __ATOMIC_SEQ_CST);
    __atomic_exchange_n(&fake_lag_outbound_budget, outbound, __ATOMIC_SEQ_CST);
}

extern "C" void fake_lag_on_tick(Hook *hook) {
    if (fake_lag_cooldown > 0)
        fake_lag_cooldown--;
    __atomic_exchange_n(&fake_lag_realtime_damage_flag, fake_lag_realtime_damage_value, __ATOMIC_SEQ_CST);
    if (fake_lag_mode == 1) {
        __atomic_exchange_n(&fake_lag_inbound_window_open, pulse_step(&fake_lag_inbound_counter, fake_lag_inbound_value), __ATOMIC_SEQ_CST);
        __atomic_exchange_n(&fake_lag_outbound_window_open, pulse_step(&fake_lag_outbound_counter, fake_lag_outbound_value), __ATOMIC_SEQ_CST);
    }

    Ctx *ctx = hook->ctx;
    if (!ctx->world || (fake_lag_require_ingame_value == 1 && !ctx->in_game)) {
        fake_lag_cooldown = 0;
        __atomic_exchange_n(&fake_lag_outbound_budget, (unsigned short)0, __ATOMIC_SEQ_CST);
        __atomic_exchange_n(&fake_lag_inbound_budget, (unsigned short)0, __ATOMIC_SEQ_CST);
        return;
    }

    int last_attack = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
    unsigned char lag;
    if (fake_lag_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player)) {
        fake_lag_cooldown = 0;
        lag = false;
    } else {
        // With "require_attack": only in the 20 ticks after an attack.
        lag = (fake_lag_require_attack_value ^ 1) | (fake_lag_cooldown != 0);
    }
    if (last_attack != fake_lag_last_hurt_time)
        fake_lag_cooldown = 20;   // we attacked
    fake_lag_last_hurt_time = last_attack;

    if (lag) {
        set_budgets((unsigned short)(int)fake_lag_inbound_value, (unsigned short)(int)fake_lag_outbound_value);
        return;
    }
    __atomic_exchange_n(&fake_lag_outbound_budget, (unsigned short)0, __ATOMIC_SEQ_CST);
    __atomic_exchange_n(&fake_lag_inbound_budget, (unsigned short)0, __ATOMIC_SEQ_CST);
}

// Every packet (network thread).
extern "C" void fake_lag_on_packet_sent(PktHook *hook) {
    unsigned short allowed_ms;
    if (hook->type == 2) {
        // Incoming.
        if (__atomic_load_n(&fake_lag_realtime_damage_flag, __ATOMIC_SEQ_CST) & 1) {
            if (java_jni_is_instance_of_s19_packet_entity_status(hook->ctx, hook->ev) ||
                java_jni_is_instance_of_s06_packet_update_health(hook->ctx, hook->ev) ||
                java_jni_is_instance_of_s29_packet_sound_effect(hook->ctx, hook->ev)) {
                hook->verdict = 3;
                return;
            }
        }
        if (__atomic_load_n(&fake_lag_inbound_window_open, __ATOMIC_SEQ_CST) & 1)
            return;
        allowed_ms = fake_lag_inbound_budget;
    } else {
        // Outgoing.
        if (__atomic_load_n(&fake_lag_outbound_window_open, __ATOMIC_SEQ_CST) & 1)
            return;
        allowed_ms = fake_lag_outbound_budget;
    }
    // Held for less than the allowed time: keep holding.
    hook->budget -= allowed_ms;
    if (hook->budget < 0) {
        hook->budget = 0;
        hook->verdict = 1;
    }
}
