// No Use Delay - stop using an item as soon as it is done.
//
// While you hold right click, Minecraft starts using the item again right after it finished
// (eating the next piece of food, ...). This module lets go of the use key for a tick once the
// item use is complete, so you can do something else straight away. With "allow_desync" it
// also lets go as soon as the client says the use is over.
//
// Callback (descriptor slot +0x168):
//   no_use_delay_on_late_tick  every tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned char no_use_delay_allow_desync_value;   // "allow_desync"

// ---- module state ----
extern "C" int no_use_delay_remaining_ticks;         // ticks left of the current item use
extern "C" int no_use_delay_last_seen_slot_state;    // hotbar slot last tick

struct DelayState { char padding_00[8]; Ctx *ctx; };  // +0x08 game context

// ---- game / engine functions ----
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *object));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(int, java_get_key_binding_key_code, (void *jvm, void *key_binding));          // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, int value));
FN(int, java_get_entity_player_item_in_use_count, (void *jvm, void *entity_player));   // ticks left of the item use

extern "C" void no_use_delay_on_late_tick(DelayState *s) {
    Ctx *ctx = s->ctx;
    if (ctx->in_game == 0) {
        no_use_delay_remaining_ticks = 0;
        return;
    }
    void *use_key = java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
    unsigned char using_key = is_key_down(ctx->jvm, use_key);

    // Switched slot: start over and give the key back.
    int slot = java_get_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player));
    if (slot != no_use_delay_last_seen_slot_state) {
        no_use_delay_remaining_ticks = 0;
        no_use_delay_last_seen_slot_state = slot;
        java_set_key_binding_pressed(ctx->jvm, use_key, java_get_key_binding_key_code(ctx->jvm, use_key));
    }
    if (!using_key)
        return;

    int use_ticks_left = java_get_entity_player_item_in_use_count(ctx->jvm, ctx->player);
    int countdown = no_use_delay_remaining_ticks;
    if (countdown <= 0) {
        // A new use started: count its length.
        no_use_delay_remaining_ticks = use_ticks_left;
        return;
    }
    countdown--;
    no_use_delay_remaining_ticks = countdown;
    // Done (or, with "allow_desync", done on the client): let go of the use key.
    if (countdown != 0 && !((use_ticks_left == 0) & no_use_delay_allow_desync_value))
        return;
    java_set_key_binding_pressed(ctx->jvm, use_key, 0);
}
