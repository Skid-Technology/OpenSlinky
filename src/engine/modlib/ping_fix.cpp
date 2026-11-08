// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
extern "C" int no_hit_delay_mode, no_use_delay_remaining_ticks, no_use_delay_last_seen_slot_state; extern "C" unsigned char no_use_delay_allow_desync_value;
struct Ev { char padding_00[8]; Ctx *ctx; void *packet; unsigned char on; int verdict; };
struct Ev2 { char padding_00[8]; void *jvm; void *packet; char padding_18[0xc]; int verdict; };
FN(int, java_get_minecraft_left_click_counter, (void *jvm, void *minecraft));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));
FN(void, java_set_minecraft_left_click_counter, (void *jvm, void *minecraft, int value));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void, java_jni_call_nonvirtual_void_method_entity_living_base_swing_item, (void *jvm, void *object));
FN(void *, java_jni_is_instance_of_s00_packet_keep_alive, (void *jvm, void *candidate));
FN(void *, java_jni_is_instance_of_c00_packet_keep_alive, (void *jvm, void *candidate));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_use_item_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *object));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inventory_player));
FN(int, java_get_key_binding_key_code, (void *jvm, void *key_binding));
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, int value));
FN(int, java_get_entity_player_item_in_use_count, (void *jvm, void *entity_player));
extern "C" int compare_by_hurt_time_desc(char *left, char *right) { return (int)(*(float *)(right + 0x30) - *(float *)(left + 0x30)); }
struct World { int *state; };
struct HitState { char pad0[8]; Ctx *ctx; unsigned char on; };
extern "C" void ping_fix_toggle(HitState *event) {
    if (event->on != 1)
        return;
    Ctx *ctx = event->ctx;
    if (*((World *)ctx->jvm)->state == 0)
        return;
    if (java_get_minecraft_left_click_counter(ctx->jvm, ctx->minecraft) <= 0)
        return;
    if (no_hit_delay_mode == 1) {
        void *t = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        if (!t || java_get_moving_object_position_type_ordinal(ctx->jvm, t) == 0)
            goto next;
    }
    java_set_minecraft_left_click_counter(ctx->jvm, ctx->minecraft, 0);
    return;
next:
    if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
        return;
    java_jni_call_nonvirtual_void_method_entity_living_base_swing_item(ctx->jvm, ctx->player);
}
extern "C" void ping_fix_packet_in(Ev2 *e) { if (java_jni_is_instance_of_s00_packet_keep_alive(e->jvm, e->packet)) e->verdict = 3; }
extern "C" void ping_fix_packet_out(Ev2 *e) { if (java_jni_is_instance_of_c00_packet_keep_alive(e->jvm, e->packet)) e->verdict = 3; }
extern "C" void ping_fix_tick(Ev *event) {
    Ctx *ctx = event->ctx;
    if (ctx->in_game == 0) {
        no_use_delay_remaining_ticks = 0;
        return;
    }
    void *it = java_get_game_settings_use_item_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft));
    unsigned char busy = is_key_down(ctx->jvm, it);
    int slot = java_get_inventory_player_current_item(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player));
    if (slot != no_use_delay_last_seen_slot_state) {
        no_use_delay_remaining_ticks = 0;
        no_use_delay_last_seen_slot_state = slot;
        void *jvm = ctx->jvm;
        java_set_key_binding_pressed(jvm, it, java_get_key_binding_key_code(jvm, it));
    }
    if (!busy)
        return;
    int w = java_get_entity_player_item_in_use_count(ctx->jvm, ctx->player);
    int cnt = no_use_delay_remaining_ticks;
    if (__builtin_expect(cnt <= 0, 0)) {
        no_use_delay_remaining_ticks = w;
        return;
    }
    cnt--;
    no_use_delay_remaining_ticks = cnt;
    if (cnt != 0) {
        unsigned char ok = (w == 0) & no_use_delay_allow_desync_value;
        __asm__("" : "+r"(ok));  // full-byte compare, as in the original
        if (ok != 1)
            return;
    }
    java_set_key_binding_pressed(ctx->jvm, it, 0);
}
