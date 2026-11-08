// Anti Debuff - remove the screen effects of blindness and nausea.
//
// Every tick: blindness has its remaining time set to 0, and the nausea (portal) wobble
// timer is reset.
//
// Callback (descriptor slot +0x168):
//   anti_debuff_on_late_tick  every tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned char anti_debuff_blindness_value;   // "blindness"
extern "C" unsigned char anti_debuff_nausea_value;      // "nausea"

// ---- game / engine functions ----
FN(void *, java_potion_potion_types, (void *jvm, int effect_id));
FN(void *, java_call_entity_living_base_get_active_potion_effect, (void *jvm, void *entity_living_base, void *effect));
FN(void, java_set_potion_effect_duration, (void *jvm, void *effect, int value));
FN(void, java_set_entity_player_sp_time_in_portal, (void *jvm, void *entity_player_sp, float value));
FN(void, java_set_entity_player_sp_prev_time_in_portal, (void *jvm, void *entity_player_sp, float value));

enum { POTION_BLINDNESS = 15 };

extern "C" void anti_debuff_on_late_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (anti_debuff_blindness_value == 1) {
        void *effect = java_call_entity_living_base_get_active_potion_effect(ctx->jvm, ctx->player, java_potion_potion_types(ctx->jvm, POTION_BLINDNESS));
        if (effect)
            java_set_potion_effect_duration(ctx->jvm, effect, 0);
    }
    if (anti_debuff_nausea_value == 1) {
        // Nausea draws the same wobble as standing in a portal.
        java_set_entity_player_sp_time_in_portal(ctx->jvm, ctx->player, 0.0f);
        java_set_entity_player_sp_prev_time_in_portal(ctx->jvm, ctx->player, 0.0f);
    }
}
