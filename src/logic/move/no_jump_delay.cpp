// No Jump Delay - jump again straight away when holding space.
//
// Minecraft waits 10 ticks between jumps while the jump key is held. This module
// resets that wait to 0 every tick.
//
// Callback (descriptor slot +0x158):
//   no_jump_delay_on_tick  every player tick
#include "slinky/engine.h"

FN(void, java_set_entity_living_base_jump_ticks, (void *jvm, void *entity_living_base, int value));

extern "C" void no_jump_delay_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game == 1)
        java_set_entity_living_base_jump_ticks(ctx->jvm, ctx->player, 0);
}
