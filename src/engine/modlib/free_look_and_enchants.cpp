// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
extern "C" char free_look_module_descriptor[]; extern "C" unsigned char free_look_active; extern "C" void *free_look_saved_camera;
extern "C" int free_look_saved_perspective, free_look_target_perspective_value;
struct Ev { char padding_00[8]; Ctx *ctx; void *tag; unsigned char on; };
FN(unsigned char, is_silent_rotation_active, ());
FN(void *, get_silent_rotation, ());
FN(void *, get_entity_rotation, (void *jvm, void *entity));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(int, java_get_game_settings_third_person_view, (void *jvm, void *game_settings));
FN(void, java_set_game_settings_third_person_view, (void *jvm, void *game_settings, int value));
FN(void, set_render_rotation, (void *cam));
FN(void *, java_call_item_stack_get_enchantment_tag_list, (void *jvm, void *stack));
FN(int, java_call_nbttag_list_tag_count, (void *jvm, void *list));
FN(void *, java_call_nbttag_list_get_compound_tag_at, (void *jvm, void *list, int i));
FN(short, java_jni_call_short_method_nbttag_compound_get_short, (void *jvm, void *tag, void *key));
struct Jvm2 { void *ids; struct JE **env; };
struct JE { char padding_00[0x538]; void *(*NewStringUTF)(void *, const char *); };
extern "C" const char str_id_in_free_look_and_enchants[], str_lvl[];
extern "C" void free_look_on_toggle(Ev *event) {
    if (event->tag != free_look_module_descriptor)
        return;
    Ctx *ctx = event->ctx;
    if (!ctx->world)
        return;
    if (event->on == 1) {
        free_look_active = 1;
        void *cam;
        if (is_silent_rotation_active())
            cam = get_silent_rotation();
        else
            cam = get_entity_rotation(ctx->jvm, ctx->player);
        free_look_saved_camera = cam;
    }
    void *x = get_game_settings(ctx->jvm, ctx->minecraft);
    void *jvm = ctx->jvm;
    if (event->on != 0) {
        free_look_saved_perspective = java_get_game_settings_third_person_view(jvm, x);
        void *jvm2 = ctx->jvm;
        unsigned persp = free_look_target_perspective_value;
        java_set_game_settings_third_person_view(jvm2, x, persp);
        return;
    }
    java_set_game_settings_third_person_view(jvm, x, free_look_saved_perspective);
    if (free_look_active == 1) {
        set_render_rotation(free_look_saved_camera);
        free_look_active = 0;
    }
}
extern "C" void free_look_save_rotation(char *p) { p[8] = 1; *(void **)(p + 0xc) = free_look_saved_camera; }

// enchantment flags of an item stack: 1 = id 16, 2 = id 19, 4 = id 20
extern "C" unsigned get_weapon_enchant_flags(Jvm2 *j, void *stack) {
    void *list = java_call_item_stack_get_enchantment_tag_list(j, stack);
    unsigned flags = 0;
    if (!list) return flags;
    JE **env = j->env;
    void *idS = (*env)->NewStringUTF(env, str_id_in_free_look_and_enchants);
    if (!idS) return flags;
    if (!(*env)->NewStringUTF(env, str_lvl)) return flags;
    int n = java_call_nbttag_list_tag_count(j, list);
    if (n <= 0) return flags;
    flags = 0;
    int i = 0;
    do {
        void *tag = java_call_nbttag_list_get_compound_tag_at(j, list, i);
        switch (java_jni_call_short_method_nbttag_compound_get_short(j, tag, idS)) {
        case 0x10: flags |= 1; break;
        case 0x13: flags |= 2; break;
        case 0x14: flags |= 4; break;
        }
    } while (++i != n);
    return flags;
}
