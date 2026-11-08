// Hitboxes - make other players' hitboxes bigger, so they are easier to hit.
//
// The engine asks every tick how much to grow hitboxes by; this answers with "expand".
//
// Callback (descriptor slot +0xc8):
//   hitboxes_on_reach_query  every tick
#include "slinky/engine.h"

// ---- settings ----
extern "C" float hitboxes_expand;   // "expand": blocks added on each side

// The engine's question, filled in by the callback.
struct HitboxState {
    char padding_00[0x10];
    bool active;          // +0x10 set to true to expand hitboxes
    char padding_11[0xf];
    double expand;        // +0x20 how much
};

extern "C" void hitboxes_on_reach_query(HitboxState *s) {
    s->active = true;
    s->expand = (double)hitboxes_expand;
}
