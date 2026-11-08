// Ghost Hand - click through entities (hit/use the block behind them).
//
// When the crosshair is on an entity, the engine asks ghost_hand_should_ignore_entity.
// If it returns 1, the entity is ignored and the click goes to whatever is behind it.
// The settings choose which held items and which kinds of entities this applies to.
//
// Callbacks:
//   ghost_hand_should_ignore_entity  engine query for one entity
//   ghost_hand_on_reach_query        descriptor slot +0xc8: mark the module as active
#include "slinky/engine.h"

// A boolean setting stored as a full setting record; the value is at +0x18.
struct Setting { char padding_00[0x18]; unsigned char on; };

// ---- settings ----
extern "C" unsigned char ghost_hand_module_enabled;
extern "C" unsigned char ghost_hand_require_mouse_setting;     // "require_mouse_pressed"
extern "C" unsigned char ghost_hand_allow_with_fists;          // "allow_using_fists"
extern "C" unsigned char ghost_hand_swords;                    // "allow_using_sword"
extern "C" unsigned char ghost_hand_tools;                     // "allow_using_tools"
extern "C" unsigned char ghost_hand_bows;                      // "allow_using_buckets"
extern "C" unsigned char ghost_hand_flint_steel;               // "allow_using_fns" (flint and steel)
extern "C" unsigned char ghost_hand_cobweb;                    // "allow_using_cobweb"
extern "C" unsigned char ghost_hand_other_items;               // "allow_using_other"
extern "C" unsigned char ghost_hand_non_living_entities;       // "non_players": ignore entities that are not players
extern "C" Setting ghost_hand_bots_setting;                    // "bots": ignore players marked as bots
extern "C" Setting ghost_hand_friendlies_setting;              // "friendlies": ignore friends and teammates
extern "C" Setting ghost_hand_enemies_setting;                 // "enemies": ignore everyone else

// ---- game / engine functions ----
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));                                     // a mouse button is held
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void *, java_jni_is_instance_of_entity_other_player_mp, (void *jvm, void *candidate));  // the entity as a player, or null
FN(unsigned, entity_flags_get, (void *jvm, void *t));                                      // bit 1/4: friend/teammate, bit 2: bot

// Is the item in your hand allowed by the settings?
static bool held_item_allowed(Ctx *ctx) {
    void *held = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (!held)
        return ghost_hand_allow_with_fists == 1;
    switch (java_call_static_item_get_id_from_item(ctx->jvm, held)) {
    // shovels, pickaxes, axes (iron, wood, stone, diamond, gold) and shears
    case 256: case 257: case 258:
    case 269: case 270: case 271:
    case 273: case 274: case 275:
    case 277: case 278: case 279:
    case 284: case 285: case 286:
    case 359:
        return ghost_hand_tools == 1;
    // swords (iron, wood, stone, diamond, gold)
    case 267: case 268: case 272: case 276: case 283:
        return ghost_hand_swords == 1;
    // bucket, water bucket, lava bucket
    case 325: case 326: case 327:
        return ghost_hand_bows == 1;
    case 259:   // flint and steel
        return ghost_hand_flint_steel == 1;
    case 30:    // cobweb
        return ghost_hand_cobweb == 1;
    default:
        return ghost_hand_other_items == 1;
    }
}

extern "C" unsigned char ghost_hand_should_ignore_entity(Ctx *ctx, void *entity) {
    if (ghost_hand_module_enabled != 1)
        return 0;
    if (ghost_hand_require_mouse_setting == 1 && !vertex_pointer_current(ctx))
        return 0;
    if (!held_item_allowed(ctx))
        return 0;

    void *player = java_jni_is_instance_of_entity_other_player_mp(ctx->jvm, entity);
    if (!player)
        return ghost_hand_non_living_entities;

    // Bots first, then friends/teammates, then everyone else.
    unsigned flags = entity_flags_get(ctx->jvm, player);
    if (flags & 2)
        return ghost_hand_bots_setting.on;
    if (flags & 5)
        return ghost_hand_friendlies_setting.on;
    return ghost_hand_enemies_setting.on;
}

extern "C" void ghost_hand_on_reach_query(char *s) {
    s[0x10] = 1;
}
