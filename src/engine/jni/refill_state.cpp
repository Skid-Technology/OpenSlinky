// 0x2b000: refill-stack tracking for the held block, accumulated rotation delta
#include "slinky/engine.h"
#include "slinky/jcall.h"

struct Rot { float yaw, pitch; };
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(int, java_get_inventory_player_current_item, (void *jvm, void *inv));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inv));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(int, java_get_item_stack_stack_size, (void *jvm, void *stack));      // stack size
FN(bool, block_entity_id, (void *jvm, void *item));      // is a block item
FN(Rot, rot_add, (Rot a, Rot b));

extern "C" int rotation_hotbar_item_id, refill_state_ticks_since_swap, refill_state_item_id, refill_state_target_stack_size, refill_state_last_slot, refill_state_countdown;
extern "C" unsigned char rotation_hotbar_item_pending;
extern "C" unsigned long long aim_assist_cached_target_ref;
extern "C" Rot aim_assist_mouse_movement_since_last_tick;

// remember the held block (id, count, slot) before placing
extern "C" void tick_click(Ctx *ctx) {
    if (refill_state_item_id != 0) return;
    void *inv = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    int slot = java_get_inventory_player_current_item(ctx->jvm, inv);
    void *items = java_get_inventory_player_main_inventory(ctx->jvm, inv);
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    jobject st = (*env)->GetObjectArrayElement(env, (jobjectArray)items, slot);
    if (!st) return;
    void *item = java_call_item_stack_get_item(ctx->jvm, st);
    if (!block_entity_id(ctx->jvm, item)) return;
    refill_state_item_id = java_call_static_item_get_id_from_item(ctx->jvm, item);
    int n = java_get_item_stack_stack_size(ctx->jvm, st);
    refill_state_target_stack_size = n;
    if ((unsigned)refill_state_ticks_since_swap > 20 || refill_state_last_slot != slot) refill_state_countdown = n;
    refill_state_ticks_since_swap = 0;
    refill_state_last_slot = slot;
}
// after placing: if the stack ran out, request a switch to the same block id
extern "C" void tick_click_done(Ctx *ctx) {
    if (refill_state_item_id == 0) return;
    void *inv = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    int slot = java_get_inventory_player_current_item(ctx->jvm, inv);
    void *items = java_get_inventory_player_main_inventory(ctx->jvm, inv);
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    jobject st = (*env)->GetObjectArrayElement(env, (jobjectArray)items, slot);
    if (st) {
        void *item = java_call_item_stack_get_item(ctx->jvm, st);
        if (item && java_call_static_item_get_id_from_item(ctx->jvm, item) != 0) {
            if (java_get_item_stack_stack_size(ctx->jvm, st) >= refill_state_target_stack_size) goto clear;
            if (--refill_state_countdown != 0) goto clear;
        }
    }
    rotation_hotbar_item_id = refill_state_item_id;
    rotation_hotbar_item_pending = 1;
clear:
    refill_state_item_id = 0;
}
extern "C" unsigned long long get_refill_state() { return aim_assist_cached_target_ref; }
extern "C" void add_rotation_delta(Rot rotation) { aim_assist_mouse_movement_since_last_tick = rot_add(aim_assist_mouse_movement_since_last_tick, rotation); }
