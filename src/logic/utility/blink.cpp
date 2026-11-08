// Blink - hold back network packets, then send them all at once.
//
// While enabled, packets are held ("direction": 0 = incoming, 1 = outgoing, 2 = both).
// To everyone else you freeze, then teleport when Blink is switched off. It switches off by
// itself after "max_duration" ticks, and optionally when you attack, dig or use a block.
//
// Callbacks (descriptor slots +0x48 / +0x68 / +0x78 / +0x98 / +0x128):
//   blink_on_tick             every tick: switch off when time is up / on attack
//   blink_on_packet_received  every incoming packet: hold it (except hit feedback)
//   blink_on_packet_sent      every outgoing packet: hold it
//   blink_on_toggle           switched on: start the timer
//   blink_on_attack           left / right click: switch off when clicking a block
#include "slinky/engine.h"

// ---- settings ----
extern "C" float blink_max_duration_value;                    // "max_duration" (ticks)
extern "C" unsigned char blink_direction;                     // "direction"
extern "C" unsigned char blink_disable_on_attack_value;       // "disable_on_attack"
extern "C" unsigned char blink_disable_on_block_dig_value;    // "disable_on_block_dig"
extern "C" unsigned char blink_disable_on_block_interact_value;   // "disable_on_block_interact"

// ---- module state ----
extern "C" char blink_module_descriptor[];
extern "C" int blink_last_hurt_time;       // tick of our last attack, last tick
extern "C" int blink_ticks_left;
extern "C" volatile unsigned char blink_active_direction;   // "direction" while active (read by the network thread)

struct PktHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    char padding_18[12];
    int verdict;            // +0x24 1 = hold, 3 = let through
};

struct PhaseHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char phase;    // +0x10 1 = left click (dig), else right click (use)
};

struct StateHook {
    char padding_00[16];
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- game / engine functions ----
FN(int, java_get_entity_living_base_last_attacker_time, (void *jvm, void *entity_living_base));   // tick of our last attack
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(void *, java_jni_is_instance_of_s19_packet_entity_status, (void *jvm, void *event));
FN(void *, java_jni_is_instance_of_s06_packet_update_health, (void *jvm, void *event));
FN(void *, java_jni_is_instance_of_s29_packet_sound_effect, (void *jvm, void *event));
FN(void *, java_jni_is_instance_of_s2_apacket_particles, (void *jvm, void *event));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 1 = a block

enum { DIRECTION_INCOMING = 0, DIRECTION_OUTGOING = 1, DIRECTION_BOTH = 2 };

extern "C" void blink_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    int last_attack = java_get_entity_living_base_last_attacker_time(ctx->jvm, ctx->player);
    unsigned char attacked = last_attack > blink_last_hurt_time;
    unsigned char disable_on_attack = blink_disable_on_attack_value;
    blink_last_hurt_time = last_attack;
    if (--blink_ticks_left <= 0 || (attacked & disable_on_attack))
        queue_module_toggle(ctx->module_manager, blink_module_descriptor, 0);
}

extern "C" void blink_on_packet_received(PktHook *hook) {
    if (blink_active_direction == DIRECTION_OUTGOING)
        return;
    // Hold everything except hurt / health / sound (hit feedback) and particles.
    int verdict = 3;
    if (!java_jni_is_instance_of_s19_packet_entity_status(hook->ctx, hook->ev) &&
        !java_jni_is_instance_of_s06_packet_update_health(hook->ctx, hook->ev) &&
        !java_jni_is_instance_of_s29_packet_sound_effect(hook->ctx, hook->ev))
        verdict = java_jni_is_instance_of_s2_apacket_particles(hook->ctx, hook->ev) ? 3 : 1;
    hook->verdict = verdict;
}

extern "C" void blink_on_packet_sent(PktHook *hook) {
    if (blink_active_direction != DIRECTION_INCOMING)
        hook->verdict = 1;
}

extern "C" void blink_on_toggle(StateHook *hook) {
    if (hook->ev != blink_module_descriptor || hook->on != 1)
        return;
    blink_ticks_left = (int)(long long)blink_max_duration_value;
    blink_last_hurt_time = 0x7fffffff;
    __atomic_exchange_n(&blink_active_direction, blink_direction, __ATOMIC_SEQ_CST);
}

extern "C" void blink_on_attack(PhaseHook *hook) {
    Ctx *ctx = hook->ctx;
    bool watch = hook->phase == 1 ? blink_disable_on_block_dig_value == 1 : blink_disable_on_block_interact_value == 1;
    if (!watch)
        return;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (hit && java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 1)
        queue_module_toggle(ctx->module_manager, blink_module_descriptor, 0);
}
