// Free Look - look around freely while you keep walking in the same direction.
//
// While the keybind is held, the camera switches to "target_perspective" (third person) and
// you can turn it without turning your player. On release, the old perspective and your
// rotation come back.
//
// Callbacks (descriptor slots +0x98 / +0x118):
//   free_look_on_module_toggle    keybind pressed / released
//   free_look_on_rotation_update  rotation hook: keep the player's rotation at the saved one
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned free_look_target_perspective_value;   // "target_perspective"

// ---- module state ----
extern "C" char free_look_module_descriptor[];
extern "C" unsigned char free_look_active;
extern "C" void *free_look_saved_camera;       // the player's rotation when free look started (yaw, pitch)
extern "C" int free_look_saved_perspective;

struct LookState {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    void *tag;              // +0x10 module descriptor whose key changed
    unsigned char held;     // +0x18 1 = pressed, 0 = released
};

// ---- game / engine functions ----
FN(bool, is_silent_rotation_active, ());
FN(void *, get_silent_rotation, ());                   // (yaw, pitch) packed in 8 bytes
FN(void *, get_entity_rotation, (void *jvm, void *entity));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(int, java_get_game_settings_third_person_view, (void *jvm, void *game_settings));
FN(void, java_set_game_settings_third_person_view, (void *jvm, void *game_settings, int perspective));
FN(void, set_render_rotation, (void *camera));

extern "C" void free_look_on_module_toggle(LookState *s) {
    if (s->tag != (void *)free_look_module_descriptor)
        return;
    Ctx *ctx = s->ctx;
    if (ctx->world == 0)
        return;

    // Pressed: remember the player's rotation.
    if (s->held == 1) {
        free_look_active = 1;
        free_look_saved_camera = is_silent_rotation_active() ? get_silent_rotation() : get_entity_rotation(ctx->jvm, ctx->player);
    }

    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    if (s->held != 0) {
        // Switch perspective (remember the old one).
        free_look_saved_perspective = java_get_game_settings_third_person_view(ctx->jvm, settings);
        java_set_game_settings_third_person_view(ctx->jvm, settings, free_look_target_perspective_value);
        return;
    }

    // Released: old perspective and rotation back.
    java_set_game_settings_third_person_view(ctx->jvm, settings, free_look_saved_perspective);
    if (free_look_active == 1) {
        set_render_rotation(free_look_saved_camera);
        free_look_active = 0;
    }
}

// Rotation hook: override the player's rotation (+0x08 flag, +0x0c rotation) with the saved one.
extern "C" void free_look_on_rotation_update(char *s) {
    s[8] = 1;
    *(void **)(s + 0xc) = free_look_saved_camera;
}
