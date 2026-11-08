// 0x47000: pick the best inventory slot for an item category and plan the swap
#include "slinky/engine.h"
#include "slinky/jids.h"
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *object, int slot));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inventory_player));
FN(unsigned, swap_delay, (Ctx *c, void *stack, unsigned kind));
struct Plan { char padding_00[0x10]; int mode; unsigned char from; unsigned char padding_15, padding_16, padding_17; char padding_18[4]; unsigned char kind, to; };
extern "C" bool plan_inventory_swap(Ctx *ctx, Plan *pl) {
    unsigned char kind = pl->kind;
    void *player = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    void *jvm = ctx->jvm;

    void *cur = java_call_iinventory_get_stack_in_slot(jvm, player, 0x27 - kind);
    void *inv = java_get_inventory_player_main_inventory(ctx->jvm, player);
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    unsigned best = swap_delay(ctx, cur, kind);
    int idx = -1;
    for (int i = 0; i != 0x24; i++) {
        unsigned v = swap_delay(ctx, (*env)->GetObjectArrayElement(env, (jobjectArray)inv, i), kind);
        if (v > best) { idx = i; best = v; }
    }
    if (idx != -1) {
        if (!cur) {
            unsigned char to = (unsigned char)(idx + 0x24);
            if (idx >= 9) to = (unsigned char)idx;
            *(long long *)&pl->mode = 0;
            pl->to = to;
        } else if (idx <= 8) {
            pl->mode = 1;
            pl->from = (unsigned char)idx;
            pl->padding_15 = 0; pl->padding_16 = 0; pl->padding_17 = 0;
            pl->to = pl->kind + 5;
        } else {
            unsigned char k = pl->kind + 5;
            pl->mode = 2;
            pl->from = k;
            pl->padding_15 = 0; pl->padding_16 = 0; pl->padding_17 = 0;
            pl->to = (unsigned char)idx;
        }
    }
    return idx != -1;
}

// armour value of a stack for slot `kind` (armour points + protection enchantments)
FN(void *, java_call_item_stack_get_item, (void *jvm, void *item_stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void *, java_call_item_stack_get_enchantment_tag_list, (void *jvm, void *stack));
FN(int, java_call_nbttag_list_tag_count, (void *jvm, void *list));
FN(void *, java_call_nbttag_list_get_compound_tag_at, (void *jvm, void *list, int i));
FN(short, java_jni_call_short_method_nbttag_compound_get_short, (void *jvm, void *tag, void *key));
extern "C" const int item_id_offsets[];
extern "C" const long long material_ids[];
extern "C" const unsigned char item_points_table[];
extern "C" const char str_id_in_chest_gui[], str_lvl_in_chest_gui[];
extern "C" const double tenth_in_inventory_swap, three_quarters, three_in_inventory_swap, swap_blend_start_weight, one_in_inventory_swap;
extern "C" const float half_in_inventory_swap, factor_1000_b;
extern "C" unsigned swap_delay(Ctx *ctx, void *stack, unsigned kind) {
    if (!stack) return 0;
    void *item = java_call_item_stack_get_item(ctx->jvm, stack);
    if (!item) return 0;
    int item_id = java_call_static_item_get_id_from_item(ctx->jvm, item);
    unsigned k = item_id - 0x12a;
    if (k > 0x13) return 0;
    if ((unsigned)(item_id + item_id_offsets[k]) != kind) return 0;
    long long mat = material_ids[k];
    void *nbt = java_call_item_stack_get_enchantment_tag_list(ctx->jvm, stack);
    unsigned char points = item_points_table[mat * 4 + kind];
    double prot = 0.0;
    if (nbt) {
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        jstring idS = (*env)->NewStringUTF(env, str_id_in_chest_gui);
        if (idS) {
            jstring lvlS = (*env)->NewStringUTF(env, str_lvl_in_chest_gui);
            if (lvlS) {
                int n = java_call_nbttag_list_tag_count(ctx->jvm, nbt);
                if (n > 0) {
                    int i = 0;
                    float extra = 0.0f;
                    double k0 = tenth_in_inventory_swap;

                    double k1 = three_quarters;

                    double k2 = three_in_inventory_swap;

                    float k3 = half_in_inventory_swap;

                    float acc = 0.0f;
                    for (; i != n; i++) {
                        void *tag = java_call_nbttag_list_get_compound_tag_at(ctx->jvm, nbt, i);
                        unsigned short eid = (unsigned short)java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, idS);
                        short lvl = java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, lvlS);
                        if (lvl < 0) continue;
                        int L = lvl;
                        switch (eid) {
                        case 1: case 2: case 3: case 4:
                            acc = (float)((double)L * k0 + (double)acc);
                            break;
                        case 0: {
                            unsigned v = (unsigned)(long long)((double)(L * L + 6) * k1 / k2);
                            if (v >= 0x19) v = 0x19;
                            v = ((v + 1) & 0x3e) + (v >> 1);
                            extra = (float)(int)v * k3;
                            break;
                        }
                        }
                    }
                    prot = (double)(acc + extra);
                }
            }
        }
    }
    double c1 = swap_blend_start_weight, c2 = one_in_inventory_swap;
    double t = c2 - (double)points * c1;
    double r = c2 - (c2 - prot * c1) * t;
    return (unsigned)(long long)((float)r * factor_1000_b);
}
