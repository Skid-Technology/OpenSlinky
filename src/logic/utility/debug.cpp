// Debug - print diagnostics into the chat.
//
//   "hurt_time":       when you get hit, the hurt time and whether you sprint / use an item
//   "item_use_state":  when the server is told you start / stop using an item
//   "clicks":          left and right clicks per tick
// Each line starts with the tick count (last 3 digits). Colours are Minecraft "§" codes.
//
// Callbacks:
//   debug_on_tick         every tick: hurt time and item use lines
//   debug_on_packet_sent  every outgoing packet: track item use start / stop
//   debug_on_attack_post  after a click: count it
//   debug_on_late_tick    every tick: click line
#include "slinky/engine.h"

// Chat line formats.
S(str_sect_7_03d_debug_hurt_time_sec);   // hurt time line
S(str_sect_7_03d_debug_item_use_sect);   // item use line
S(str_astart);                           // "aStart" (the format puts "§" before it: green "Start")
S(str_cstop);                            // "cStop"  (red "Stop")
S(str_sect_7_03d_debug_click_sect_cl);   // click line
extern "C" const int debug_hurt_time_code_table[];   // colour code for hurt time 7, 8, 9, 10: 'a', 'e', 'c', '4'

// ---- settings ----
extern "C" unsigned char debug_hurt_time_value;        // "hurt_time"
extern "C" unsigned char debug_item_use_state_value;   // "item_use_state"
extern "C" unsigned char debug_clicks_value;           // "clicks"

// ---- module state ----
extern "C" unsigned char debug_last_item_use_state;
extern "C" unsigned char debug_current_item_use_state;   // written by the network thread
extern "C" int debug_left_clicks_this_window;
extern "C" int debug_right_clicks_this_window;
extern "C" int debug_ticks_window;                       // ticks since the last click

struct EvHook {
    char padding_00[8];
    void *ctx;              // +0x08 JVM handle
    void *ev;               // +0x10 the packet
    char padding_18[12];
    int type;               // +0x24
};

// ---- game / engine functions ----
FN(int, java_get_entity_living_base_hurt_time, (void *jvm, void *entity_living_base));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void, jni_throw_format, (void *jvm, const char *fmt, ...));                          // print a line into the chat
FN(void *, java_jni_is_instance_of_c08_packet_player_block_placement, (void *ctx, void *event));   // "use item" packet
FN(void *, java_jni_is_instance_of_c07_packet_player_digging, (void *ctx, void *event));
FN(int, java_c07_packet_player_digging_status, (void *ctx, void *object));                          // 5 = release use item

// Minecraft colour codes.
enum { GREEN = 'a', AQUA = 'b', RED = 'c', WHITE = 'f', DARK_GREEN = '2', DARK_AQUA = '3', DARK_GRAY = '8' };

// The tick count, last 3 digits.
static unsigned tick_stamp(void *jvm, void *player) {
    unsigned ticks = java_get_entity_ticks_existed(jvm, player);
    return ticks - ticks / 1000 * 1000;
}

extern "C" void debug_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;

    if (debug_hurt_time_value == 1) {
        int hurt_time = java_get_entity_living_base_hurt_time(ctx->jvm, ctx->player);
        if (hurt_time >= 7) {
            void *jvm = ctx->jvm;
            unsigned stamp = tick_stamp(jvm, ctx->player);
            int hurt_colour = WHITE;
            unsigned index = hurt_time - 7;
            if (index <= 3)
                hurt_colour = debug_hurt_time_code_table[index];
            int sprint_colour = RED;
            if (java_call_entity_is_sprinting(ctx->jvm, ctx->player))
                sprint_colour = DARK_GRAY;
            int item_colour = DARK_GRAY;
            if (java_call_entity_player_is_using_item(ctx->jvm, ctx->player))
                item_colour = RED;
            jni_throw_format(jvm, str_sect_7_03d_debug_hurt_time_sec, stamp, hurt_colour, hurt_time, sprint_colour, item_colour);
        }
    }

    if (debug_item_use_state_value == 1) {
        unsigned state = debug_current_item_use_state;
        unsigned char using_item = state & 1;
        if (using_item != debug_last_item_use_state) {
            debug_last_item_use_state = using_item;
            void *jvm = ctx->jvm;
            unsigned stamp = tick_stamp(jvm, ctx->player);
            jni_throw_format(jvm, str_sect_7_03d_debug_item_use_sect, stamp, (state & 1) ? str_astart : str_cstop);
        }
    }
}

// Every outgoing packet: "use item" = start, "release use item" = stop.
extern "C" void debug_on_packet_sent(EvHook *hook) {
    int type = hook->type;
    if (type != 3 && type != 0)
        return;
    unsigned char using_item = 1;
    if (!java_jni_is_instance_of_c08_packet_player_block_placement(hook->ctx, hook->ev)) {
        void *dig = java_jni_is_instance_of_c07_packet_player_digging(hook->ctx, hook->ev);
        if (!dig)
            return;
        if (java_c07_packet_player_digging_status(hook->ctx, dig) != 5)
            return;
        using_item = 0;
    }
    __atomic_exchange_n(&debug_current_item_use_state, using_item, __ATOMIC_SEQ_CST);
}

// After a click (h[0x10]: 1 = left, 0 = right).
extern "C" void debug_on_attack_post(unsigned char *h) {
    if (debug_clicks_value != 1)
        return;
    if (h[0x10])
        debug_left_clicks_this_window += 1;
    else
        debug_right_clicks_this_window += 1;
    debug_ticks_window = 0;
}

// Every tick: print the clicks of this tick (for up to 10 ticks after the last click).
extern "C" void debug_on_late_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game != 1)
        return;
    unsigned ticks = debug_ticks_window;
    if (debug_clicks_value == 1) {
        if (ticks <= 9) {
            // Colour: grey for 0, light for 1, dark for 2+.
            int left = debug_left_clicks_this_window;
            int left_colour = DARK_GREEN;
            if (left == 1)
                left_colour = GREEN;
            if (left == 0)
                left_colour = DARK_GRAY;
            int right = debug_right_clicks_this_window;
            int right_colour = DARK_AQUA;
            if (right == 1)
                right_colour = AQUA;
            if (right == 0)
                right_colour = DARK_GRAY;
            void *jvm = ctx->jvm;
            unsigned stamp = tick_stamp(jvm, ctx->player);
            jni_throw_format(jvm, str_sect_7_03d_debug_click_sect_cl, stamp, left_colour, debug_left_clicks_this_window, right_colour, debug_right_clicks_this_window);
            ticks = debug_ticks_window;
        }
        debug_left_clicks_this_window = 0;
        debug_right_clicks_this_window = 0;
    }
    debug_ticks_window = ticks + 1;
}
