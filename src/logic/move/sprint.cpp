// Sprint - sprints for you, and keeps you sprinting in the directions the settings allow.
//
// The module has two callbacks (descriptor slots +0x158 and +0x198):
//   sprint_on_player_update   every player tick: holds the sprint key down for you
//   sprint_on_movement_input  every movement update: boosts speed when sprinting sideways/backwards/while eating
#include "slinky/engine.h"

// ---- settings (defined in sprint.h, values live in the original data section) ----
extern "C" unsigned char sprint_while_in_gui_value;      // "while_in_gui":     keep sprinting while a menu is open
extern "C" unsigned char sprint_while_using_item_value;  // "while_using_item": sprint while eating/blocking/drawing a bow
extern "C" unsigned char sprint_while_backwards_value;   // "while_backwards":  sprint when walking backwards
extern "C" unsigned char sprint_while_sideways_value;    // "while_sideways":   sprint when only strafing

// 1 while this module is the reason the sprint key is held down.
extern "C" unsigned char sprint_sprint_currently_forced;

// ---- what the engine hands to each callback ----
struct SprintState {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context (JVM, Minecraft instance, local player)
};

struct MoveState {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    float strafe;        // +0x10 sideways input   (-1 right .. +1 left, 0 = none)
    float forward;       // +0x14 forwards input   (-1 back  .. +1 forward, 0 = none)
    float speed;         // +0x18 movement speed for this tick, can be changed
};

// ---- game functions used (implemented in the engine, they call into Minecraft over JNI) ----
FN(void *, get_current_screen, (void *jvm, void *object));                          // open menu, or null
FN(void *, get_game_settings, (void *jvm, void *object));                           // Minecraft's GameSettings
FN(void *, java_get_game_settings_key_17_or_8, (void *jvm, void *game_settings));   // the sprint key binding
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(void, java_call_entity_set_sprinting, (void *jvm, void *entity, bool on));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));

// Every player tick: hold the sprint key down whenever sprinting is allowed.
extern "C" void sprint_on_player_update(SprintState *s) {
    Ctx *ctx = s->ctx;

    // Sprinting is allowed when no menu is open, or when "while_in_gui" is on.
    bool menu_open = get_current_screen(ctx->jvm, ctx->minecraft) != nullptr;
    bool allowed = !menu_open || (sprint_while_in_gui_value & 1);

    // Press (or release) the sprint key to match.
    void *game_settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *sprint_key = java_get_game_settings_key_17_or_8(ctx->jvm, game_settings);
    java_set_key_binding_pressed(ctx->jvm, sprint_key, allowed);

    void *player = ctx->player;
    if (player) {
        if (!allowed) {
            // A menu just opened: stop the sprint we were forcing.
            // (The "forced" flag stays set until sprinting is allowed again.)
            if (sprint_sprint_currently_forced == 1)
                java_call_entity_set_sprinting(ctx->jvm, player, false);
            return;
        }
        // Already sprinting: set it again so the game does not drop it.
        if (java_call_entity_is_sprinting(ctx->jvm, player))
            java_call_entity_set_sprinting(ctx->jvm, ctx->player, true);
    }
    sprint_sprint_currently_forced = allowed;
}

// Every movement update: Minecraft only sprints forwards and not while using an item.
// For the extra directions the settings allow, sprint anyway and add the sprint speed boost (x1.3).
extern "C" void sprint_on_movement_input(MoveState *s) {
    Ctx *ctx = s->ctx;

    // Standing still: nothing to do.
    if (s->forward == 0.0f && s->strafe == 0.0f)
        return;

    bool using_item = java_call_entity_player_is_using_item(ctx->jvm, ctx->player);
    if (using_item && !sprint_while_using_item_value)
        return;

    if (s->forward < 0.0f) {
        // Walking backwards.
        if (!sprint_while_backwards_value)
            return;
    } else if (s->forward == 0.0f) {
        // Only strafing (we know strafe != 0 here).
        if (!sprint_while_sideways_value)
            return;
    } else {
        // Walking forwards: the game already sprints by itself, unless an item is being used.
        if (!using_item)
            return;
    }

    s->speed = (float)(s->speed * 1.3);
    java_call_entity_set_sprinting(ctx->jvm, ctx->player, true);
}
