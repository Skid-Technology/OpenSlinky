// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
FN(void, java_set_entity_living_base_jump_ticks, (void *jvm, void *entity_living_base, int value));
extern "C" void no_jump_delay_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game == 1) java_set_entity_living_base_jump_ticks(ctx->jvm, ctx->player, 0);
}
