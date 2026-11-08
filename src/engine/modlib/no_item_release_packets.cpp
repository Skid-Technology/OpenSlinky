// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
extern "C" int no_item_release_mode; extern "C" unsigned char no_item_release_release_lock_flag; extern "C" float hitboxes_expand;
struct Ev { char padding_00[8]; void *jvm; void *pkt; char padding_18[0xc]; int verdict; };
FN(void *, java_jni_is_instance_of_c08_packet_player_block_placement, (void *w, void *candidate));
FN(void *, java_get_c08_packet_player_block_placement_stack, (void *w, void *c08_packet_player_block_placement));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *item_stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void *, java_jni_is_instance_of_c07_packet_player_digging, (void *w, void *candidate));
FN(int, java_c07_packet_player_digging_status, (void *w, void *object));
extern "C" void no_item_release_on_send_packet(Ev *event) {
    unsigned char lock = 0;
    void *w = java_jni_is_instance_of_c08_packet_player_block_placement(event->jvm, event->pkt);
    void *rw = event->jvm;
    if (w) {
        void *t = java_get_c08_packet_player_block_placement_stack(rw, w);
        if (t) {
            void *item = java_call_item_stack_get_item(rw, t);
            if (item) {
                int id = java_call_static_item_get_id_from_item(rw, item);
                bool compute;
                switch (id) {
                case 267: case 268: case 272: case 276: case 283:  // swords
                    compute = no_item_release_mode != 1;
                    break;
                case 261:  // bow
                case 346:
                    compute = false;
                    break;
                default:
                    compute = no_item_release_mode != 0;
                    break;
                }
                if (compute) {
                    int id2 = java_call_static_item_get_id_from_item(rw, item);
                    lock = (id2 != 0x15a) & (id2 != 0x105);
                }
            }
        }
        no_item_release_release_lock_flag = lock;
        return;
    }
    w = java_jni_is_instance_of_c07_packet_player_digging(rw, event->pkt);
    if (!w || no_item_release_release_lock_flag == 0)
        return;
    if (java_c07_packet_player_digging_status(event->jvm, w) == 5)
        event->verdict = 2;
    no_item_release_release_lock_flag = 0;
}
// original engine copy of hitboxes_on_reach_query; the descriptor uses the logic function (dead duplicate)
extern "C" void hitboxes_apply_hitbox_expansion_original(char *p) { p[0x10] = 1; *(double *)(p + 0x20) = hitboxes_expand; }
