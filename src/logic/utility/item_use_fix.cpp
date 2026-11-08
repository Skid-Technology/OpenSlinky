// Item Use Fix - let you use an item while mining.
//
// Minecraft won't use an item (right click) while you are hitting a block. Just before the
// right click is handled, this module clears the "hitting a block" flag, and puts it back
// right after.
//
// Callbacks (descriptor slots +0x128 / +0x138):
//   item_use_fix_clear_hitting_block    before the right click
//   item_use_fix_restore_hitting_block  after the right click
#include "slinky/engine.h"

// ---- module state ----
extern "C" unsigned char item_use_fix_item_use_flag;   // 1: we cleared the flag and must restore it

struct UseState {
    char padding_00[8];
    Ctx *ctx;              // +0x08 game context
    unsigned char skip;    // +0x10 non-zero: not a real click
};

// ---- game / engine functions ----
FN(void *, java_get_minecraft_player_controller, (void *jvm, void *minecraft));
FN(unsigned char, java_get_player_controller_mp_is_hitting_block, (void *jvm, void *player_controller_mp));
FN(void, java_set_player_controller_mp_is_hitting_block, (void *jvm, void *player_controller_mp, int on));

extern "C" void item_use_fix_clear_hitting_block(UseState *s) {
    if (s->skip != 0)
        return;
    Ctx *ctx = s->ctx;
    void *controller = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
    unsigned char hitting = java_get_player_controller_mp_is_hitting_block(ctx->jvm, controller);
    item_use_fix_item_use_flag = hitting != 0;
    if (hitting)
        java_set_player_controller_mp_is_hitting_block(ctx->jvm, controller, 0);
}

extern "C" void item_use_fix_restore_hitting_block(UseState *s) {
    if (item_use_fix_item_use_flag != 1 || s->skip != 0)
        return;
    Ctx *ctx = s->ctx;
    void *controller = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
    java_set_player_controller_mp_is_hitting_block(ctx->jvm, controller, item_use_fix_item_use_flag);
}
