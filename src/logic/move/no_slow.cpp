// No Slow - walk faster while using an item (bow, sword block, eating/drinking).
//
// Minecraft multiplies your movement input by 0.2 while you use an item. This module
// replaces that 0.2 with the multiplier from the settings for that kind of item.
//
// Callback (descriptor slot +0x198):
//   no_slow_on_movement_input  every movement update, after the game has slowed the input
#include "slinky/engine.h"

// ---- settings ----
extern "C" float no_slow_mult_bow_value;           // "mult_bow"
extern "C" float no_slow_mult_sword_value;         // "mult_sword"
extern "C" float no_slow_mult_consumables_value;   // "mult_consumables": food, potions, everything else

struct SlowState {
    char padding_00[8];
    Ctx *ctx;          // +0x08 game context
    float move[2];     // +0x10 strafe and forward input (already multiplied by 0.2)
};

// ---- game / engine functions ----
FN(bool, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));

// Item ids (Minecraft 1.8).
enum {
    ITEM_BOW = 261,
    ITEM_IRON_SWORD = 267,
    ITEM_WOODEN_SWORD = 268,
    ITEM_STONE_SWORD = 272,
    ITEM_DIAMOND_SWORD = 276,
    ITEM_GOLDEN_SWORD = 283,
};

extern "C" void no_slow_on_movement_input(SlowState *s) {
    Ctx *ctx = s->ctx;
    if (!java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
        return;
    void *item = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (!item)
        return;

    // Pick the multiplier for this item.
    const float *mult;
    switch (java_call_static_item_get_id_from_item(ctx->jvm, item)) {
    case ITEM_BOW:
        mult = &no_slow_mult_bow_value;
        break;
    case ITEM_IRON_SWORD:
    case ITEM_WOODEN_SWORD:
    case ITEM_STONE_SWORD:
    case ITEM_DIAMOND_SWORD:
    case ITEM_GOLDEN_SWORD:
        mult = &no_slow_mult_sword_value;
        break;
    default:
        mult = &no_slow_mult_consumables_value;
        break;
    }

    // Undo the game's 0.2 and apply ours instead.
    double factor = *mult / 0.2;
    s->move[0] = (float)(s->move[0] * factor);
    s->move[1] = (float)(s->move[1] * factor);
}
