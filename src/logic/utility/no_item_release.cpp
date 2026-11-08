// No Item Release - keep using an item (eating, blocking, ...) on the server after you let go.
//
// When you stop using an item, the game sends a "release use item" packet. This module
// cancels that packet for the items chosen by "mode", so the server thinks you are still
// eating / blocking.
//   mode 0: swords only    mode 1: everything except swords    other: everything
// Bows and fishing rods are never affected (releasing is how they work).
//
// Callback (descriptor slot +0x78):
//   no_item_release_on_packet_sent  every outgoing packet
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned no_item_release_mode;   // "mode"

// ---- module state ----
// 1 after a "use item" packet for an affected item: cancel the next release.
extern "C" unsigned char no_item_release_release_lock_flag;

struct RelState {
    char padding_00[8];
    void *w;                // +0x08 JVM handle
    void *x;                // +0x10 the packet
    char padding_18[0xc];
    int state;              // +0x24 set to 2 to cancel the packet
};

// ---- game / engine functions ----
FN(void *, java_jni_is_instance_of_c08_packet_player_block_placement, (void *w, void *candidate));   // "use item" packet, or null
FN(void *, java_get_c08_packet_player_block_placement_stack, (void *w, void *c08_packet_player_block_placement));
FN(void *, java_call_item_stack_get_item, (void *w, void *item_stack));
FN(int, java_call_static_item_get_id_from_item, (void *w, void *item));
FN(void *, java_jni_is_instance_of_c07_packet_player_digging, (void *w, void *candidate));            // dig / release packet, or null
FN(int, java_c07_packet_player_digging_status, (void *w, void *object));                              // 5 = release use item

enum { ITEM_BOW = 261, ITEM_FISHING_ROD = 346 };

extern "C" void no_item_release_on_packet_sent(RelState *s) {
    void *use_packet = java_jni_is_instance_of_c08_packet_player_block_placement(s->w, s->x);
    void *jvm = s->w;

    if (use_packet) {
        // Starting to use an item: should its release be cancelled?
        unsigned char lock = 0;
        void *stack = java_get_c08_packet_player_block_placement_stack(jvm, use_packet);
        if (stack) {
            void *item = java_call_item_stack_get_item(jvm, stack);
            if (item) {
                bool affected;
                switch (java_call_static_item_get_id_from_item(jvm, item)) {
                case 267: case 268: case 272: case 276: case 283:   // swords
                    affected = no_item_release_mode != 1;
                    break;
                case ITEM_BOW:
                case ITEM_FISHING_ROD:
                    affected = false;
                    break;
                default:
                    affected = no_item_release_mode != 0;
                    break;
                }
                if (affected) {
                    int id = java_call_static_item_get_id_from_item(jvm, item);
                    lock = (id != ITEM_FISHING_ROD) & (id != ITEM_BOW);
                }
            }
        }
        no_item_release_release_lock_flag = lock;
        return;
    }

    // The release: cancel it once.
    void *dig_packet = java_jni_is_instance_of_c07_packet_player_digging(jvm, s->x);
    if (!dig_packet || no_item_release_release_lock_flag == 0)
        return;
    if (java_c07_packet_player_digging_status(s->w, dig_packet) == 5)
        s->state = 2;
    no_item_release_release_lock_flag = 0;
}
