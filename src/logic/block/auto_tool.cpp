// Auto Tool - switch to the best tool for the block you are mining.
//
// While you hold the attack key on a block, the hotbar slot that mines it fastest is
// selected. With "switch_back" on, your old slot is selected again when you stop.
//
// Callbacks (descriptor slots +0x1d8 / +0x1e8):
//   auto_tool_on_inventory_tick  every tick
//   auto_tool_on_shutdown        module unloaded: free the allowed-block list
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };

// ---- settings ----
extern "C" float auto_tool_activation_time_value;              // "activation_time": ticks to wait before switching
extern "C" unsigned char auto_tool_allow_using_fists_value;    // "allow_using_fists": may switch away from an empty hand
extern "C" unsigned char auto_tool_allow_using_sword_value;    // "allow_using_sword": may switch away from a sword
extern "C" unsigned char auto_tool_allow_using_tools_value;    // "allow_using_tools": may switch away from a tool
extern "C" unsigned char auto_tool_allow_using_other_value;    // "allow_using_other": may switch away from anything else
extern "C" unsigned char auto_tool_require_sneak_value;        // "require_sneak"
extern "C" unsigned char auto_tool_restrict_allowed_blocks;    // "restrict_allowed_blocks": only for blocks in the list
extern "C" unsigned char auto_tool_switch_back_value;          // "switch_back"
extern "C" void *auto_tool_allowed_block_set[2];               // the block list (start, end of a vector of ids)

// ---- module state ----
extern "C" char auto_tool_module_descriptor[];
extern "C" int auto_tool_slot_switch_back;      // slot to go back to afterwards, -1 = none
extern "C" int auto_tool_slot_we_switched;      // slot we selected
extern "C" int auto_tool_activation_countdown;  // ticks left before we may switch

// ---- game / engine functions ----
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));        // selected hotbar slot
FN(void, java_set_inventory_player_current_item, (void *jvm, void *inventory_player, int slot));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, java_get_game_settings_attack_key, (void *jvm, void *game_settings));
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));            // key physically held down
FN(void *, java_get_game_settings_sneak_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *k));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));             // what the crosshair points at
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));   // 1 = a block
FN(Vec3, java_get_moving_object_position_block_x, (void *jvm, void *moving_object_position));
FN(void *, java_call_variant, (void *jvm, void *world, int x, int y, int z));               // block at x, y, z
FN(int, get_block_id, (void *jvm, void *blk));
FN(unsigned char, id_vector_contains, (void *set, int id));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(float, java_call_item_stack_get_str_vs_block, (void *jvm, void *item_stack, void *blk));  // mining speed of the item on the block
FN(unsigned char, claim_release_hotbar_module, (void *timer));   // ask for control of the hotbar (one module at a time)
FN(void, release_owner, (void *timer));                           // give control of the hotbar back
FN(void, libc_free, (void *ptr));

// Is the module allowed to switch away from the item in your hand?
static bool may_switch_from_held_item(Ctx *ctx) {
    void *held = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (!held)
        return auto_tool_allow_using_fists_value == 1;
    switch (java_call_static_item_get_id_from_item(ctx->jvm, held)) {
    // shovels, pickaxes, axes (iron, wood, stone, diamond, gold) and shears
    case 256: case 257: case 258:
    case 269: case 270: case 271:
    case 273: case 274: case 275:
    case 277: case 278: case 279:
    case 284: case 285: case 286:
    case 359:
        return auto_tool_allow_using_tools_value == 1;
    // swords (iron, wood, stone, diamond, gold)
    case 267: case 268: case 272: case 276: case 283:
        return auto_tool_allow_using_sword_value == 1;
    default:
        return auto_tool_allow_using_other_value == 1;
    }
}

// Give the hotbar back, and select the old slot again if we remembered one.
static void stop_and_switch_back(Ctx *ctx, void *inventory) {
    release_owner(auto_tool_module_descriptor);
    if (auto_tool_slot_switch_back != -1) {
        java_set_inventory_player_current_item(ctx->jvm, inventory, auto_tool_slot_switch_back);
        auto_tool_slot_switch_back = -1;
    }
}

extern "C" void auto_tool_on_inventory_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    int current_slot = java_get_inventory_player_current_item(ctx->jvm, inventory);

    // The player picked another slot themselves: forget the slot to switch back to.
    if (auto_tool_slot_switch_back != -1 && auto_tool_slot_we_switched != current_slot) {
        auto_tool_slot_switch_back = -1;
        stop_and_switch_back(ctx, inventory);
        return;
    }

    // Check that we should be active. If not, restart the wait and switch back.
    bool active = true;
    void *block = nullptr;
    if (auto_tool_slot_switch_back == -1 && !may_switch_from_held_item(ctx))
        active = false;
    if (active) {
        void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
        if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player)) {
            active = false;
        } else if (!java_get_key_binding_key_code(ctx->jvm, java_get_game_settings_attack_key(ctx->jvm, settings))) {
            active = false;   // not mining
        } else if (auto_tool_require_sneak_value == 1 && !is_key_down(ctx->jvm, java_get_game_settings_sneak_key(ctx->jvm, settings))) {
            active = false;
        } else {
            void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
            if (!hit || java_get_moving_object_position_type_ordinal(ctx->jvm, hit) != 1) {
                active = false;   // not looking at a block
            } else {
                Vec3 pos = java_get_moving_object_position_block_x(ctx->jvm, hit);
                block = java_call_variant(ctx->jvm, ctx->world, (int)pos.x, (int)pos.y, (int)pos.z);
                if (!block)
                    active = false;
                else if (auto_tool_restrict_allowed_blocks == 1 && !id_vector_contains(auto_tool_allowed_block_set, get_block_id(ctx->jvm, block)))
                    active = false;
            }
        }
    }
    if (!active) {
        auto_tool_activation_countdown = (int)auto_tool_activation_time_value;
        stop_and_switch_back(ctx, inventory);
        return;
    }

    // Still waiting for "activation_time".
    if (auto_tool_activation_countdown > 0) {
        auto_tool_activation_countdown = auto_tool_activation_countdown - 1;
        stop_and_switch_back(ctx, inventory);
        return;
    }

    // Find the hotbar slot that mines this block fastest (starting from the current slot,
    // so on a tie the current one wins). Speed 1.0 is the bare hand.
    int best_slot = current_slot;
    float best_speed = 1.0f;
    for (int i = 0; i < 9; i++) {
        int slot = (current_slot + i) % 9;
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, slot);
        if (!stack)
            continue;
        float speed = java_call_item_stack_get_str_vs_block(ctx->jvm, stack, block);
        if (speed > best_speed)
            best_slot = slot;
        best_speed = speed > best_speed ? speed : best_speed;
    }
    if (best_slot == current_slot)
        return;

    // Switch, if no other module is using the hotbar right now.
    if (claim_release_hotbar_module(auto_tool_module_descriptor)) {
        java_set_inventory_player_current_item(ctx->jvm, inventory, best_slot);
        auto_tool_slot_we_switched = best_slot;
        if (auto_tool_slot_switch_back == -1 && auto_tool_switch_back_value)
            auto_tool_slot_switch_back = current_slot;
    }
}

extern "C" void auto_tool_on_shutdown(Hook *hook) {
    libc_free(auto_tool_allowed_block_set[0]);
    auto_tool_allowed_block_set[0] = auto_tool_allowed_block_set[1] = nullptr;
}
