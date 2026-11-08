// helper functions (formerly address block; names inferred from behaviour).
// cflags: -mllvm -switch-to-lookup=false
#include "slinky/engine.h"
FN(void *, java_get_minecraft_player_controller, (void *jvm, void *minecraft));
FN(unsigned char, java_get_player_controller_mp_is_hitting_block, (void *jvm, void *player_controller_mp));
FN(void, java_set_player_controller_mp_is_hitting_block, (void *jvm, void *player_controller_mp, int on));
extern "C" unsigned char item_use_fix_item_use_flag, timer_range_active_flag, timer_range_timer_modified;
extern "C" int timer_range_delay_ticks;
struct Ev { char padding_00[8]; Ctx *ctx; unsigned char on; };
extern "C" void item_use_fix_on_attack(Ev *event) {
    if (__builtin_expect(event->on != 0, 0))
        goto out;
    {
        Ctx *ctx = event->ctx;
        void *minecraft = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
        unsigned char usable = java_get_player_controller_mp_is_hitting_block(ctx->jvm, minecraft);
        item_use_fix_item_use_flag = usable != 0;
        if (usable) {
            java_set_player_controller_mp_is_hitting_block(ctx->jvm, minecraft, 0);
            return;
        }
    }
out:
    EPILOG_NOP();
}
extern "C" void item_use_fix_on_use(Ev *event) {
    if (item_use_fix_item_use_flag != 1 || event->on != 0) return;
    Ctx *ctx = event->ctx;
    void *minecraft = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
    void *jvm = ctx->jvm;

    java_set_player_controller_mp_is_hitting_block(jvm, minecraft, item_use_fix_item_use_flag);
}
extern "C" bool timer_range_is_delay_pending() {
    unsigned char r;
    if (timer_range_active_flag == 1) { asm volatile(""); r = (timer_range_delay_ticks > 0) | timer_range_timer_modified; }
    else r = 0;
    SHARED_EPILOG();
    return r & 1;
}

FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, item_held_hotbar_slot, (void *jvm, void *slot));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
extern "C" float no_slow_mult_sword_value, no_slow_mult_bow_value, no_slow_mult_consumables_value;
extern "C" double divisor_0_2_in_item_use_fix_and_no_slow;
struct Ev2 { char padding_00[8]; Ctx *ctx; float x, y; };
extern "C" void no_slow_item_use_speed(Ev2 *ev) {
    Ctx *ctx = ev->ctx;
    if (!java_call_entity_player_is_using_item(ctx->jvm, ctx->player)) return;
    void *item = item_held_hotbar_slot(ctx->jvm, ctx->player);
    if (!item) return;
    float *f;
    switch (java_call_static_item_get_id_from_item(ctx->jvm, item)) {
    case 0x10b: case 0x10c: case 0x110: case 0x114: case 0x11b: f = &no_slow_mult_sword_value; break;   // swords
    case 0x105: f = &no_slow_mult_bow_value; break;
    default: f = &no_slow_mult_consumables_value; break;
    }
    double s = (double)*f / divisor_0_2_in_item_use_fix_and_no_slow;
    ev->x = (float)(s * ev->x);
    ev->y = (float)(s * ev->y);
}