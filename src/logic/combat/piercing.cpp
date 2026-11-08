// Piercing - hit through entities and blocks.
//
// When two entities are under the crosshair, the engine asks which one the attack should hit.
// With this module the attack can go through friends, bots and non-players to reach an
// enemy behind them, and (with "blocks") through blocks too.
//
// Functions:
//   piercing_decide_which_entity   0 = hit the first entity, 1 = hit the second, 2 = no preference
//   piercing_should_pierce_blocks  true while block piercing is active
//   piercing_on_reach_query        mark the module as active
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned char piercing_module_enabled;
extern "C" unsigned char piercing_require_inside_bb_value;   // "require_inside_bb": only when standing inside the first entity's hitbox
extern "C" unsigned char piercing_require_weapon_value;      // "require_weapon"
extern "C" unsigned char piercing_non_players_value;         // "non_players": hit through non-players
extern "C" bool piercing_friendlies_value;                   // "friendlies": hit through friends and teammates
extern "C" bool piercing_bots_value;                         // "bots": hit through bots
extern "C" unsigned char piercing_enemies_value;             // "enemies": choose between two enemies
extern "C" int piercing_sort_by_value;                       // "sort_by": 0 = hurt time, 1 = health
extern "C" unsigned char piercing_blocks_value;              // "blocks": hit through blocks

// ---- game / engine functions ----
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_jni_is_instance_of_entity_other_player_mp, (void *jvm, void *candidate));   // the entity as a player, or null
FN(unsigned, entity_flags_get, (void *jvm, void *t));                                       // bit 1/4: friend/teammate, bit 2: bot
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(int, java_get_entity_entity_id, (void *jvm, void *entity));

// Scrambles a number (a simple hash), used to break ties in a stable but random-looking way.
static unsigned mix(unsigned x) {
    x = (x ^ (x >> 16)) * 0x45d9f3b;
    x = (x ^ (x >> 16)) * 0x45d9f3b;
    return x ^ (x >> 16);
}

enum { HIT_FIRST = 0, HIT_SECOND = 1, NO_PREFERENCE = 2 };

extern "C" int piercing_decide_which_entity(Ctx *ctx, void *first, void *second, unsigned char inside_bb) {
    if (piercing_module_enabled != 1)
        return NO_PREFERENCE;
    if (!inside_bb && piercing_require_inside_bb_value)
        return NO_PREFERENCE;
    if (piercing_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return NO_PREFERENCE;

    void *second_player = java_jni_is_instance_of_entity_other_player_mp(ctx->jvm, second);
    if (!second_player)
        return 2 - piercing_non_players_value;   // "non_players" on: HIT_SECOND, else NO_PREFERENCE

    // The second one is a friend or a bot we may hit through.
    unsigned flags = entity_flags_get(ctx->jvm, second_player);
    if ((piercing_friendlies_value & ((flags & 5) != 0)) || (piercing_bots_value & ((flags & 2) >> 1)))
        return HIT_SECOND;

    void *first_player = java_jni_is_instance_of_entity_other_player_mp(ctx->jvm, first);
    if (!first_player)
        return HIT_FIRST;
    if (piercing_enemies_value != 1)
        return NO_PREFERENCE;

    // Two enemies: pick by the "sort_by" setting.
    if (piercing_sort_by_value == 1) {
        float first_health = java_call_entity_living_base_get_health(ctx->jvm, first_player);
        float second_health = java_call_entity_living_base_get_health(ctx->jvm, second_player);
        if (second_health > first_health)
            return HIT_SECOND;
        if (first_health > second_health)
            return HIT_FIRST;
    } else if (piercing_sort_by_value == 0) {
        int first_hurt = java_get_entity_living_base_hurt_time(ctx->jvm, first_player);
        int second_hurt = java_get_entity_living_base_hurt_time(ctx->jvm, second_player);
        if (second_hurt > first_hurt)
            return HIT_SECOND;
        if (second_hurt < first_hurt)
            return HIT_FIRST;
    }

    // Still a tie: pick one by a hash of age + id.
    unsigned first_hash = mix(java_get_entity_ticks_existed(ctx->jvm, first_player) + java_get_entity_entity_id(ctx->jvm, first_player));
    unsigned second_hash = mix(java_get_entity_ticks_existed(ctx->jvm, second_player) + java_get_entity_entity_id(ctx->jvm, second_player));
    return (int)second_hash <= (int)first_hash;
}

extern "C" unsigned char piercing_should_pierce_blocks(Ctx *ctx) {
    if (piercing_module_enabled != 1 || !piercing_blocks_value)
        return 0;
    if (piercing_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return 0;
    return 1;
}

extern "C" void piercing_on_reach_query(unsigned char *h) {
    h[0x10] = 1;
}
