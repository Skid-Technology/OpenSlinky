// Knockback Displace - push your target sideways instead of straight back.
//
// Knockback goes the way you are facing. On a hit, this module turns your head sideways by
// "angle" degrees (with a little randomness) for that one tick, so the target flies off to
// the side. Next tick it turns back and clicks again. "hide_rotation" holds outgoing packets
// during that tick so the turn is not sent separately.
//
// Callbacks (descriptor slots +0x78 / +0x118 / +0x128 / +0x138 / +0x168):
//   knockback_displace_on_packet_sent      every outgoing packet: hold it while hiding the turn
//   knockback_displace_on_rotation_update  rotation hook: see below
//   knockback_displace_on_attack           before an attack: start the sideways turn
//   knockback_displace_on_attack_post      after an attack
//   knockback_displace_on_late_tick        every tick: turn back, bookkeeping
//   knockback_displace_direction           helper: -1 = left, +1 = right
#include "slinky/engine.h"

struct Rot { float yaw, pitch; };
struct Range { float lo, hi; };

struct AttackHook {
    char padding_00[8];
    Ctx *ctx;                // +0x08 game context
    unsigned char phase;     // +0x10 1 = the attack is about to happen
    unsigned char handled;   // +0x11 set to 1 to cancel the attack (we click again next tick)
};
struct PktHook { char padding_00[0x24]; int verdict; };   // +0x24 set to 1 to hold the packet

// ---- settings ----
extern "C" float knockback_displace_cooldown_value;                // "cooldown": ticks between displacements
extern "C" float knockback_displace_angle;                         // "angle": degrees to turn
extern "C" unsigned knockback_displace_direction_value;            // "direction": 0 left, 1 right, 2 random, 3 strafe, 4 against strafe
extern "C" unsigned char knockback_displace_use_last_strafe_value; // "use_last_strafe": remember the strafe direction after letting go
extern "C" unsigned char knockback_displace_hide_rotation_value;   // "hide_rotation"
extern "C" unsigned char knockback_displace_require_no_kb_value;   // "require_no_kb"
extern "C" unsigned char knockback_displace_require_weapon_value;  // "require_weapon"
extern "C" unsigned char knockback_displace_require_kb_enchant_value;   // "require_kb_enchant"
extern "C" unsigned char knockback_displace_require_facing_away_value;  // "require_facing_away"

// ---- module state ----
extern "C" unsigned char knockback_displace_rotation_hidden;      // holding packets (read by the network thread)
extern "C" unsigned char knockback_displace_displacing_this_tick;
extern "C" unsigned char knockback_displace_displaced_last_tick;
extern "C" unsigned char knockback_displace_hit_seen_this_tick;
extern "C" unsigned knockback_displace_ticks_since_reset;
extern "C" unsigned knockback_displace_ticks_since_displacement;
extern "C" Rot knockback_displace_rotation_before_displacement;   // rotation to turn back to
extern "C" int knockback_displace_last_strafe_sign;               // -1 right, 0 none, +1 left

// ---- game / engine functions ----
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 2 = an entity
FN(void *, java_get_moving_object_position_entity_hit, (void *jvm, void *moving_object_position));
FN(void *, java_jni_is_instance_of_entity_other_player_mp, (void *jvm, void *candidate));   // the entity as a player, or null
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mode));                   // mode 7: friend, teammate or bot
FN(unsigned char, timer_range_is_delay_pending, ());                                        // Timer Range is busy
FN(unsigned char, is_held_item_accepted_weapon, (void *jvm, void *item, unsigned *flags));  // flags & 2: knockback enchant
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(unsigned char, yaw_diff_check, (Ctx *ctx, Rot rotation, void *target));                 // facing away from the target
FN(unsigned char, update_rotation_priority_peak, (int ms));                                 // may this module control the rotation?
FN(void, apply_player_rotation, (Ctx *ctx, unsigned char a, int b, int d, Rot rotation));
FN(unsigned char, random_chance, (float chance));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, random_rotation, (Range r));                                                       // random value in the range
FN(void, apply_silent_rotation, (Rot rotation));
FN(void, set_rotation_active, (void *jvm, void *object, int x));
FN(float, java_get_entity_living_base_move_strafing, (void *jvm, void *entity_living_base));
FN(Rot, get_silent_rotation_offset, ());
FN(Rot, rot_add, (Rot a, Rot b));
FN(Rot, rot_sub, (Rot a, Rot b));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void, java_set_minecraft_left_click_counter, (void *jvm, void *minecraft, int y));
FN(void *, java_get_game_settings_attack_key, (void *jvm, void *game_settings));
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));            // = one click
extern "C" int knockback_displace_direction();

extern "C" void knockback_displace_on_packet_sent(PktHook *hook) {
    if (__atomic_load_n(&knockback_displace_rotation_hidden, __ATOMIC_SEQ_CST) & 1)
        hook->verdict = 1;
}

// While displacing (this tick or last tick): clear the engine's pitch-override flag (+0x09).
extern "C" void knockback_displace_on_rotation_update(char *h) {
    if (knockback_displace_displacing_this_tick != 0 || knockback_displace_displaced_last_tick != 0)
        h[9] = 0;
}

extern "C" void knockback_displace_on_attack(AttackHook *hook) {
    // Already turned this tick: cancel further attacks until we turn back.
    if (knockback_displace_displacing_this_tick == 1) {
        hook->handled = 1;
        return;
    }
    if (hook->phase != 1)
        return;
    Ctx *ctx = hook->ctx;

    // Attacking a player we may fight?
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 2)
        return;
    void *target = java_jni_is_instance_of_entity_other_player_mp(ctx->jvm, java_get_moving_object_position_entity_hit(ctx->jvm, hit));
    if (!target || entity_flags_has(ctx->jvm, target, 7))
        return;
    knockback_displace_hit_seen_this_tick = 1;

    // Conditions.
    if (knockback_displace_displaced_last_tick || knockback_displace_ticks_since_reset < 9)
        return;
    if (knockback_displace_cooldown_value > (float)knockback_displace_ticks_since_displacement)
        return;
    if (timer_range_is_delay_pending())
        return;
    if (knockback_displace_require_no_kb_value == 1 && ((CombatTarget *)ctx->target)->valid)
        return;
    unsigned flags;
    unsigned char weapon = is_held_item_accepted_weapon(ctx->jvm, ctx->player, &flags);
    if (knockback_displace_require_weapon_value == 1 && !weapon)
        return;
    // The hit must deal extra knockback: a knockback enchant, or sprinting.
    if (!(flags & 2) && (knockback_displace_require_kb_enchant_value || !java_call_entity_is_sprinting(ctx->jvm, ctx->player)))
        return;
    Rot rotation = get_entity_rotation(ctx->jvm, ctx->player);
    knockback_displace_rotation_before_displacement = rotation;
    if (knockback_displace_require_facing_away_value == 1 && !yaw_diff_check(ctx, rotation, target))
        return;
    int direction = knockback_displace_direction();
    if (!direction || !update_rotation_priority_peak(200))
        return;

    // Turn sideways: "angle" minus 0..10 degrees, and pitch 10..20 degrees up or down at random.
    apply_player_rotation(ctx, 1, 0, 0, knockback_displace_rotation_before_displacement);
    knockback_displace_displacing_this_tick = 1;
    unsigned char pitch_down = random_chance(0.5f);
    float yaw = (knockback_displace_angle - random_rotation(rot_make_floats(0.0f, 10.0f))) * (float)direction;
    float pitch = random_rotation(rot_make_floats(10.0f, 20.0f));
    if (!pitch_down)
        pitch = -pitch;
    apply_silent_rotation(Rot{yaw, pitch});
    set_rotation_active(ctx->jvm, ctx->player, 0);
    if (knockback_displace_hide_rotation_value == 1)
        __atomic_exchange_n(&knockback_displace_rotation_hidden, (unsigned char)1, __ATOMIC_SEQ_CST);

    // Cancel this attack; the click is repeated next tick, after the turn was sent.
    hook->handled = 1;
    knockback_displace_ticks_since_displacement = 0;
}

extern "C" void knockback_displace_on_attack_post(AttackHook *hook) {
    if (hook->phase == 1 && knockback_displace_ticks_since_reset >= 10 && (knockback_displace_hit_seen_this_tick & 1))
        knockback_displace_ticks_since_reset = 0;
}

extern "C" void knockback_displace_on_late_tick(Hook *hook) {
    knockback_displace_ticks_since_reset += 1;
    knockback_displace_ticks_since_displacement += 1;
    knockback_displace_displaced_last_tick = knockback_displace_displacing_this_tick;
    knockback_displace_hit_seen_this_tick = 0;

    Ctx *ctx = hook->ctx;
    if (ctx->in_game == 1) {
        // Remember which way we strafe (for "direction" 3 and 4).
        float strafe = java_get_entity_living_base_move_strafing(ctx->jvm, ctx->player);
        int sign = (strafe > 0.0f) - (0.0f > strafe);
        if (knockback_displace_use_last_strafe_value != 1 || sign != 0)
            knockback_displace_last_strafe_sign = sign;

        // We turned last tick: turn back, and click again (the attack that was cancelled).
        if (knockback_displace_displacing_this_tick == 1 && update_rotation_priority_peak(200)) {
            Rot now = rot_add(get_entity_rotation(ctx->jvm, ctx->player), get_silent_rotation_offset());
            apply_silent_rotation(rot_sub(knockback_displace_rotation_before_displacement, now));
            set_rotation_active(ctx->jvm, ctx->player, 0);
            void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
            java_set_minecraft_left_click_counter(ctx->jvm, ctx->minecraft, 0);
            java_increment_key_binding_press_time(ctx->jvm, java_get_game_settings_attack_key(ctx->jvm, settings));
        }
    }
    knockback_displace_displacing_this_tick = 0;

    // Keep holding packets for the tick after the turn, if "hide_rotation".
    unsigned char hide = knockback_displace_displaced_last_tick ? knockback_displace_hide_rotation_value : 0;
    __atomic_exchange_n(&knockback_displace_rotation_hidden, hide, __ATOMIC_SEQ_CST);
}

// -1 = turn left, +1 = turn right, 0 = don't.
extern "C" int knockback_displace_direction() {
    switch (knockback_displace_direction_value) {
    case 0:
        return -1;
    case 1:
        return 1;
    case 2:
        return (random_chance(0.5f) - 1) | 1;   // -1 or +1 at random
    case 3:
        return knockback_displace_last_strafe_sign;
    case 4:
        return -knockback_displace_last_strafe_sign;
    default:
        __builtin_unreachable();   // the setting only has 5 options
    }
}
