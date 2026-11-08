// 0x32000: key-name table helpers and a "speed" module's callbacks
#include "slinky/engine.h"
struct Vec3 { double x, y, z; };

extern "C" int key_colour_count;
extern "C" char key_colour_vector_header[], gui_state_block[];
extern "C" void *teams_colour_table;
extern "C" long long gui_accent_color_setting_field_0x8; extern "C" int gui_accent_color_setting_field_0x10;
extern "C" const char *teams_team_colors_setting_field_0x70[16];
extern "C" char gui_module_descriptor[], timer_module_descriptor[];
extern "C" float timer_speed_value, one_in_key_names, timer_speed_1_in_key_names_a;
extern "C" unsigned char timer_require_mouse_value, timer_require_falling_value, timer_require_damage_value;

extern "C" int libc_strcmp(const char *left, const char *right);
FN(unsigned long long, libc_strlen, (const char *str));
FN(void **, element_at, (void *vec, int elem, void *hdr, void *key));
FN(void *, vector_find_insert, (void *t, int elem, void *hdr, void *e, void *cmp));
FN(void, vector_erase, (void *vec, int elem, void *hdr, void *key));
FN(unsigned char, is_unload_requested, (void *x));
FN(void *, get_timer, (void *jvm, void *timer));
FN(void, java_set_timer_timer_speed, (void *jvm, void *timer, float speed));
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));
FN(unsigned char, vertex_pointer_current, (Ctx *ctx));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));

extern "C" int compare_signed_chars(const signed char *, const signed char *right, const signed char *b) { return *right - *b; }
extern "C" void key_filter(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    if (n > 99) return;
    unsigned char l = (unsigned char)(character - 0x61);
    asm("" : "+r"(l));
    if (l >= 0x1a) {
        unsigned char u = (unsigned char)(character - 0x41);
    asm("" : "+r"(u));
        unsigned char notupper = u >= 0x1a;
    asm("" : "+r"(notupper));
        unsigned char notspace = character != 0x20;
    asm("" : "+r"(notspace));
        if (notupper & notspace) return;
    }
    buf[n] = character;
}
extern "C" int get_key_name_index() { return key_colour_count; }
extern "C" const char *key_name_for(void *, void *key) {
    signed char *p = (signed char *)element_at(teams_colour_table, 1, key_colour_vector_header, key);
    return teams_team_colors_setting_field_0x70[*p];
}
extern "C" void remove_key_name(void *, void *key) { vector_erase(teams_colour_table, 1, key_colour_vector_header, key); }
extern "C" void clear_key_name() { key_colour_count = 0; }
extern "C" void key_name_table(const char ***a, long long *n) { *a = teams_team_colors_setting_field_0x70; *n = 0x10; }
extern "C" unsigned char is_unload_requested_flag() { return is_unload_requested(gui_state_block); }
struct Pair { long long a; int b; };
extern "C" Pair get_speed_pair() { Pair r; r.b = gui_accent_color_setting_field_0x10; r.a = gui_accent_color_setting_field_0x8; return r; }
extern "C" bool enable_hud_editor(Ctx **h) { queue_module_toggle(h[1], gui_module_descriptor, 1); return true; }

struct Hk { char padding_00[8]; Ctx *ctx; };
extern "C" bool key_name_matches(void *, const char *name) {
    int i;
    for (i = 0; i < 16; i++)
        if (libc_strcmp(name, teams_team_colors_setting_field_0x70[i]) == 0) break;
    if (i == 16) return false;
    signed char idx = (signed char)i;
    vector_find_insert(&teams_colour_table, 1, key_colour_vector_header, &idx, (void *)compare_signed_chars);
    return true;
}
extern "C" void speed_hook_tick(Hk *h) {
    Ctx *ctx = h->ctx;
    void *timer = get_timer(ctx->jvm, ctx->minecraft);
    float speed;
    if (timer_require_mouse_value == 1 && !vertex_pointer_current(ctx)) goto slow;
    if (timer_require_falling_value == 1) {
        if (java_get_entity_on_ground(ctx->jvm, ctx->player)) goto slow;
        Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
        if (motion.y >= 0.0) goto slow;
    }
    if (timer_require_damage_value == 1) {
        if (ctx->in_game != 1) goto slow;
        if (!java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player)) goto slow;
    }
    speed = timer_speed_value;
    goto go;
slow:
    speed = one_in_key_names;
go:
    java_set_timer_timer_speed(ctx->jvm, timer, speed);
}
extern "C" void speed_hook_enable(Hk *h) {
    if (*(void **)((char *)h + 0x10) == timer_module_descriptor && *((char *)h + 0x18) == 0) {
        Ctx *ctx = h->ctx;
        void *timer = get_timer(ctx->jvm, ctx->minecraft);
        void *jvm = ctx->jvm;

        java_set_timer_timer_speed(jvm, timer, timer_speed_1_in_key_names_a);
    }
}

FN(void, empty_placeholder, (void *x));
extern "C" void reset_key_state() { empty_placeholder(gui_state_block); }
