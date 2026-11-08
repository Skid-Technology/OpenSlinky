// Null Move - pressing a movement key overrides the opposite one.
//
// Normally holding A and D together cancels out and you stand still. With this module the
// key you pressed last wins: when you press a key while its opposite is held, the opposite
// key is released. When you let go again, the opposite key is restored if you still hold it.
//
// Callback (descriptor slot +0x148):
//   null_move_on_tick  every client tick
#include "slinky/engine.h"

// One value per key (forward, left, back, right): the tick the key was pressed, or 0 when not held.
extern "C" int null_move_key_state_0;   // forward
extern "C" int null_move_key_state_1;   // left
extern "C" int null_move_key_state_2;   // back
extern "C" int null_move_key_state_3;   // right

// ---- game / engine functions ----
FN(unsigned char, is_in_game, (void *jvm, void *object));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *k));                                  // key binding pressed (by the game)
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));

// Handle one key and its opposite.
static void handle_key(Ctx *ctx, int *state, void *key, void *opposite_key, int now) {
    int pressed_at = *state;
    bool key_down = is_key_down(ctx->jvm, key);
    bool opposite_down = is_key_down(ctx->jvm, opposite_key);

    if (key_down) {
        // Just pressed while the opposite is held: release the opposite.
        bool just_pressed = pressed_at == 0;
        if (just_pressed && opposite_down)
            java_set_key_binding_pressed(ctx->jvm, opposite_key, 0);
        *state = now;
    } else if (pressed_at == 0) {
        *state = 0;
    } else {
        // Just let go: give the opposite key back if it is still physically held.
        unsigned char opposite_held = java_get_key_binding_key_code(ctx->jvm, opposite_key);
        *state = 0;
        if (opposite_held)
            java_set_key_binding_pressed(ctx->jvm, opposite_key, 1);
    }
}

extern "C" void null_move_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (!is_in_game(ctx->jvm, ctx->minecraft))
        return;
    int now = java_get_entity_ticks_existed(ctx->jvm, ctx->player);

    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *forward_key = java_get_game_settings_forward_key(ctx->jvm, settings);
    void *left_key = java_get_game_settings_left_key(ctx->jvm, settings);
    void *back_key = java_get_game_settings_back_key(ctx->jvm, settings);
    void *right_key = java_get_game_settings_right_key(ctx->jvm, settings);

    handle_key(ctx, &null_move_key_state_0, forward_key, back_key, now);
    handle_key(ctx, &null_move_key_state_1, left_key, right_key, now);
    handle_key(ctx, &null_move_key_state_2, back_key, forward_key, now);
    handle_key(ctx, &null_move_key_state_3, right_key, left_key, now);
}
