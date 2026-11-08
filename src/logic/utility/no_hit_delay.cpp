// No Hit Delay - remove the wait after a missed click.
//
// When you click at nothing, Minecraft blocks attacks for 10 ticks. This module resets that
// wait, so every click counts.
//   mode 0: always
//   mode 1: only when the crosshair is on something; clicks at nothing just swing the arm
//
// Callback (descriptor slot +0x128):
//   no_hit_delay_on_attack  before an attack
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned no_hit_delay_mode;   // "mode"

struct HitState {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char on;       // +0x10 1 = an attack click
};

// ---- game / engine functions ----
FN(int, java_get_minecraft_left_click_counter, (void *jvm, void *minecraft));            // ticks attacks are blocked
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 0 = nothing
FN(void, java_set_minecraft_left_click_counter, (void *jvm, void *minecraft, int value));
FN(bool, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void, java_jni_call_nonvirtual_void_method_entity_living_base_swing_item, (void *jvm, void *object));

extern "C" void no_hit_delay_on_attack(HitState *s) {
    if (s->on != 1)
        return;
    Ctx *ctx = s->ctx;
    if (**(int **)ctx->jvm == 0)
        return;   // the JVM handle is not ready
    if (java_get_minecraft_left_click_counter(ctx->jvm, ctx->minecraft) <= 0)
        return;   // not blocked

    if (no_hit_delay_mode == 1) {
        void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) == 0) {
            // Clicking at nothing: only swing the arm.
            if (!java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
                java_jni_call_nonvirtual_void_method_entity_living_base_swing_item(ctx->jvm, ctx->player);
            return;
        }
    }
    java_set_minecraft_left_click_counter(ctx->jvm, ctx->minecraft, 0);
}
