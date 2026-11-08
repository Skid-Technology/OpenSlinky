// Ping Fix - drop keep-alive packets in both directions.
//
// The server measures your ping with keep-alive packets. Dropping them stops it from
// measuring (and from seeing the delays other modules add).
//
// Callbacks (descriptor slots +0x68 / +0x78):
//   ping_fix_drop_incoming_keepalive  every incoming packet
//   ping_fix_drop_outgoing_keepalive  every outgoing packet
#include "slinky/engine.h"

struct PingState {
    char padding_00[8];
    void *jvm;          // +0x08 JVM handle
    void *packet;       // +0x10 the packet
    char padding_18[0xc];
    int state;          // +0x24 set to 3 to drop the packet
};

FN(void *, java_jni_is_instance_of_s00_packet_keep_alive, (void *jvm, void *candidate));
FN(void *, java_jni_is_instance_of_c00_packet_keep_alive, (void *jvm, void *candidate));

extern "C" void ping_fix_drop_incoming_keepalive(PingState *s) {
    if (java_jni_is_instance_of_s00_packet_keep_alive(s->jvm, s->packet))
        s->state = 3;
}

extern "C" void ping_fix_drop_outgoing_keepalive(PingState *s) {
    if (java_jni_is_instance_of_c00_packet_keep_alive(s->jvm, s->packet))
        s->state = 3;
}
