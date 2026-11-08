// Auto Weapon - switch to your weapon when you aim at a player.
//
// When the crosshair is on a player you may fight (not a friend/teammate/bot) and you are
// not holding a weapon, the first weapon slot in the hotbar is selected, after the crosshair
// has been on players for "activation_time" ticks. "Weapon" is decided by the Weapons module.
//
// Callback (descriptor slot +0x1d8):
//   auto_weapon_on_inventory_tick  every tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" float auto_weapon_activation_time_value;               // "activation_time"
extern "C" unsigned char auto_weapon_require_mouse_pressed_value; // "require_mouse_pressed"

// ---- module state ----
extern "C" int auto_weapon_ticks_since_target_acquired;
extern "C" char auto_weapon_module_descriptor[];

struct WState { char padding_00[8]; Ctx *ctx; };   // +0x08 game context

// ---- game / engine functions ----
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                       // a mouse button is held
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 2 = an entity
FN(void *, java_get_moving_object_position_entity_hit, (void *jvm, void *moving_object_position));
FN(void *, java_jni_is_instance_of_entity_player, (void *jvm, void *candidate));            // the entity as a player, or null
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mode));                   // mode 7: friend, teammate or bot
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(int, weapons_find_first_weapon_slot_entry, (Ctx *ctx, void *player));                   // see weapons.cpp
FN(unsigned char, claim_release_hotbar_module, (void *timer));                               // ask for control of the hotbar
FN(void, java_set_inventory_player_current_item, (void *jvm, void *player, int slot));
FN(void, release_owner, (void *timer));                                                       // give control of the hotbar back

// Is the crosshair on a player we may fight, while we are not holding a weapon?
static bool should_switch(Ctx *ctx) {
    if (ctx->in_game != 1)
        return false;
    if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
        return false;
    if (auto_weapon_require_mouse_pressed_value == 1 && !vertex_pointer_current(ctx))
        return false;
    void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
    if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 2)
        return false;
    void *target = java_jni_is_instance_of_entity_player(ctx->jvm, java_get_moving_object_position_entity_hit(ctx->jvm, hit));
    if (!target || entity_flags_has(ctx->jvm, target, 7))
        return false;
    if (is_holding_accepted_weapon(ctx->jvm, ctx->player))
        return false;
    return true;
}

extern "C" void auto_weapon_on_inventory_tick(WState *s) {
    Ctx *ctx = s->ctx;
    if (!should_switch(ctx)) {
        auto_weapon_ticks_since_target_acquired = 0;
        return;
    }

    void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    int current_slot = java_get_inventory_player_current_item(ctx->jvm, inventory);
    int weapon_slot = weapons_find_first_weapon_slot_entry(ctx, inventory);
    if (weapon_slot == -1 || current_slot == weapon_slot) {
        auto_weapon_ticks_since_target_acquired = 0;
        return;
    }

    // Wait "activation_time" ticks first.
    int ticks = auto_weapon_ticks_since_target_acquired;
    if (auto_weapon_activation_time_value > (float)ticks) {
        auto_weapon_ticks_since_target_acquired = ticks + 1;
        return;
    }

    // Switch, if no other module is using the hotbar right now.
    if (!claim_release_hotbar_module(auto_weapon_module_descriptor))
        return;
    java_set_inventory_player_current_item(ctx->jvm, inventory, weapon_slot);
    release_owner(auto_weapon_module_descriptor);
}
