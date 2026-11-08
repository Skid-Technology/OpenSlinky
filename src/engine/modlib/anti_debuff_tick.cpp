// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
FN(void *, java_potion_potion_types, (void *jvm, int id));
FN(void *, java_call_entity_living_base_get_active_potion_effect, (void *jvm, void *entity_living_base, void *effect));
FN(void, java_set_potion_effect_duration, (void *jvm, void *effect, int value));
FN(void, java_set_entity_player_sp_time_in_portal, (void *jvm, void *entity_player_sp, float value));
FN(void, java_set_entity_player_sp_prev_time_in_portal, (void *jvm, void *entity_player_sp, float value));
extern "C" unsigned char anti_debuff_blindness_value, anti_debuff_nausea_value;
struct Ev { char padding_00[8]; Ctx *ctx; };
extern "C" void anti_debuff_tick(Ev *event) {
    Ctx *ctx = event->ctx;
    if (anti_debuff_blindness_value == 1) {
        void *jvm = java_potion_potion_types(ctx->jvm, 0xf);
        void *effect = java_call_entity_living_base_get_active_potion_effect(ctx->jvm, ctx->player, jvm);
        if (effect) java_set_potion_effect_duration(ctx->jvm, effect, 0);
    }
    if (anti_debuff_nausea_value == 1) {
        java_set_entity_player_sp_time_in_portal(ctx->jvm, ctx->player, 0.0f);
        java_set_entity_player_sp_prev_time_in_portal(ctx->jvm, ctx->player, 0.0f);
    }
}
