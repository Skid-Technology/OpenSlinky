// Fast Mine - break blocks faster.
//
// Two tricks:
//  - the pause between breaking one block and starting the next ("block hit delay") is
//    capped at "break_delay", and can count down faster;
//  - on the first tick of mining a block, the progress is jumped ahead so the block
//    breaks in 1/"speed" of the normal number of ticks.
//
// Callbacks (descriptor slots +0x48 / +0x168):
//   fast_mine_on_tick       every tick: shorten the block hit delay
//   fast_mine_on_late_tick  every tick, after mining progress was updated: skip ahead
#include "slinky/engine.h"

// ---- settings ----
extern "C" float fast_mine_break_delay_value;                  // "break_delay": longest delay allowed (ticks)
extern "C" unsigned char fast_mine_decr_block_hit_delay_value; // "decr_block_hit_delay": count the delay down twice as fast
extern "C" float fast_mine_speed;                              // "speed": 2 = blocks break in half the ticks

// ---- module state ----
extern "C" int fast_mine_last_block_hit_delay;    // block hit delay seen last tick
extern "C" float fast_mine_last_progress_value;   // mining progress seen last tick (0 = not mining)

// ---- game / engine functions ----
FN(void *, java_get_minecraft_player_controller, (void *jvm, void *minecraft));
FN(float, java_get_player_controller_mp_cur_block_damage_mp, (void *jvm, void *blk));          // mining progress, 0..1
FN(void, java_set_player_controller_mp_cur_block_damage_mp, (void *jvm, void *blk, float value));
FN(int, java_get_player_controller_mp_block_hit_delay, (void *jvm, void *blk));
FN(void, java_set_player_controller_mp_block_hit_delay, (void *jvm, void *blk, int value));
FN(float, libc_ceilf, (float v));

extern "C" void fast_mine_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    int delay = 0;
    void *controller = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
    if (controller) {
        fast_mine_last_progress_value = java_get_player_controller_mp_cur_block_damage_mp(ctx->jvm, controller);
        delay = java_get_player_controller_mp_block_hit_delay(ctx->jvm, controller);
        if ((float)delay > fast_mine_break_delay_value) {
            // Longer than allowed: cap it.
            java_set_player_controller_mp_block_hit_delay(ctx->jvm, controller, (int)fast_mine_break_delay_value);
        } else if (delay > 0 && fast_mine_decr_block_hit_delay_value && delay == fast_mine_last_block_hit_delay) {
            // The game did not count it down this tick yet: take one off ourselves.
            java_set_player_controller_mp_block_hit_delay(ctx->jvm, controller, delay - 1);
        }
    }
    fast_mine_last_block_hit_delay = delay;
}

extern "C" void fast_mine_on_late_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    void *controller = java_get_minecraft_player_controller(ctx->jvm, ctx->minecraft);
    if (!controller)
        return;

    // Only on the first tick of mining a block (progress went from 0 to something).
    float progress_per_tick = java_get_player_controller_mp_cur_block_damage_mp(ctx->jvm, controller);
    if (progress_per_tick == 0.0f || fast_mine_last_progress_value != 0.0f)
        return;

    // Ticks the block normally takes, and ticks we want it to take.
    long long normal_ticks = (long long)libc_ceilf(1.0f / progress_per_tick);
    double wanted = 1.0 / (double)fast_mine_speed / (double)progress_per_tick;
    long long wanted_ticks = (long long)libc_ceilf((float)wanted);

    // Jump ahead by the difference.
    unsigned int ticks_done = (unsigned int)((int)normal_ticks - (int)wanted_ticks + 1);
    java_set_player_controller_mp_cur_block_damage_mp(ctx->jvm, controller, (float)(long long)ticks_done * progress_per_tick);
}
