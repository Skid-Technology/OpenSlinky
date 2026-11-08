// Auto Clicker - click for you while you hold the attack button.
//
// While the attack mouse button is held, the module sends its own button presses and
// releases at "cps" clicks per second, with human-like randomness ("randomize"): slow drifts
// in speed, occasional bursts of fast clicks, and in "mode" 1 occasional double clicks.
// In inventories ("inventory_fill"), holding shift and dragging over slots shift-clicks them.
//
// Callbacks:
//   auto_clicker_tick              every tick: find the attack button, reset the click delay
//   auto_clicker_on_player_update  every player tick: what is under the crosshair
//   auto_clicker_on_frame          every frame: send presses / releases when they are due
//   auto_clicker_extra_burst_clicks  helper: occasional bursts of faster clicks
#include "slinky/engine.h"

struct Rot { float yaw, pitch; };
struct Range { float lo, hi; };

struct TickHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    char padding_10[8];
    double dt;           // +0x18 seconds since the last frame
};

// ---- settings ----
extern "C" float auto_clicker_cps_setting;                     // "cps"
extern "C" unsigned char auto_clicker_randomize_setting;       // "randomize"
extern "C" unsigned auto_clicker_pattern;                      // "mode": 0 = even clicks, 1 = with double clicks
extern "C" unsigned char auto_clicker_burst_setting;           // "burst"
extern "C" unsigned char auto_clicker_break_blocks_setting;    // "break_blocks": don't click on blocks (so they can be mined)
extern "C" unsigned char auto_clicker_require_weapon_value;    // "require_weapon"
extern "C" unsigned char auto_clicker_pause_when_using_setting;    // "pause_when_using"
extern "C" unsigned char auto_clicker_allow_inventory;         // "inventory_fill"
extern "C" unsigned char auto_clicker_inventory_random_delay_setting;   // "inventory_random_delay"

// ---- module state ----
extern "C" Rot auto_clicker_rotation_at_last_tick;
extern "C" unsigned char auto_clicker_attack_bound_mouse;      // 0 = attack is the left button, 1 = the right button
extern "C" int auto_clicker_hit_type_under_crosshair;          // 0 = nothing, 1 = block, 2 = entity
extern "C" unsigned char *const ptr_to_no_hit_delay_module_descriptor;   // client state; +0x14 = a screen is open
extern "C" double auto_clicker_time_until_next_click_edge;     // seconds until the next press/release
extern "C" double auto_clicker_last_edge_delay;
extern "C" int auto_clicker_burst_clicks_left;
extern "C" unsigned char auto_clicker_double_click_pending;
extern "C" unsigned char auto_clicker_lmb_was_down_gui;
extern "C" int auto_clicker_last_inventory_slot_clicked;
// Random walk / random number states.
extern "C" char auto_clicker_rng_states[], auto_clicker_rng_states_second[], auto_clicker_rng_states_third[];
extern "C" char auto_clicker_random_walk_1[], auto_clicker_random_walk_2[], auto_clicker_random_walk_3[], auto_clicker_random_walk_4[],
                auto_clicker_random_walk_5[], auto_clicker_random_walk_6[], auto_clicker_random_walk_7[], auto_clicker_gauss_rng_state[];

// ---- game / engine functions ----
FN(float, gauss_random, (void *st));                                          // normally distributed random number
FN(Range, rot_make_floats, (float lo, float hi));
FN(unsigned char, walk_should_change, (void *state, Range r, double p));     // random yes/no
FN(float, walk_update, (void *state, Range r, double p));                    // next value of a random walk
FN(float, random_rotation, (Range r));                                        // random value in the range
FN(void *, get_timer, (void *jvm, void *timer));
FN(float, java_get_timer_timer_speed, (void *jvm, void *timer));
FN(void, post_synthetic_click, (int button, unsigned char down));            // press / release a mouse button
FN(float, rot_delta_length, (Rot a, Rot b));                                  // how far the view turned
FN(unsigned char, is_holding_accepted_weapon, (void *jvm, void *object));
FN(unsigned char, java_call_entity_player_is_using_item, (void *jvm, void *entity_player));
FN(void *, get_current_screen, (void *jvm, void *object));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_get_gui_container_the_slot, (void *jvm, void *screen));      // slot under the mouse
FN(void *, java_get_inventory_player_item_stack, (void *jvm, void *inv));    // item on the mouse cursor
FN(int, java_get_slot_slot_number, (void *jvm, void *slot));
FN(void *, java_call_slot_get_stack, (void *jvm, void *slot));
FN(unsigned char, random_chance, (float chance));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_attack_key, (void *jvm, void *game_settings));
FN(int, get_event_key, (void *jvm, void *k));                                 // key code; -100 = left mouse, -99 = right mouse
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(void, java_set_minecraft_left_click_counter, (void *jvm, void *minecraft, int y));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(int, java_get_moving_object_position_type_ordinal, (void *jvm, void *moving_object_position));

enum { KEY_LEFT_MOUSE = -100, KEY_RIGHT_MOUSE = -99 };

static int auto_clicker_attack_key;

extern "C" void auto_clicker_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    auto_clicker_attack_key = get_event_key(ctx->jvm, java_get_game_settings_attack_key(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));
    auto_clicker_attack_bound_mouse = auto_clicker_attack_key == KEY_RIGHT_MOUSE;
    if (ctx->in_game != 1)
        return;
    auto_clicker_rotation_at_last_tick = get_entity_rotation(ctx->jvm, ctx->player);
    if (ptr_to_no_hit_delay_module_descriptor[0x14])
        return;
    // Remove Minecraft's 10-tick wait after a missed click.
    java_set_minecraft_left_click_counter(ctx->jvm, ctx->minecraft, 0);
}

extern "C" void auto_clicker_on_player_update(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->in_game == 1) {
        void *hit = java_get_minecraft_object_mouse_over(ctx->jvm, ctx->minecraft);
        auto_clicker_hit_type_under_crosshair = hit ? java_get_moving_object_position_type_ordinal(ctx->jvm, hit) : 0;
    }
}

// Now and then, a burst of 1-3 quicker clicks. Returns 1 (and makes *delay a bit longer
// ...the burst clicks are spaced 0.02 s + noise apart) while a burst is running.
extern "C" unsigned char auto_clicker_extra_burst_clicks(float *delay, float base) {
    int clicks = auto_clicker_burst_clicks_left;
    if (clicks <= 0) {
        // Start a new burst? More likely while aiming around ("base").
        if (auto_clicker_burst_setting != 1)
            return 0;
        double chance = base;
        Range start_chance = {(float)(0.02 + chance), (float)(chance + 0.04)};
        if (!walk_should_change(auto_clicker_rng_states, start_chance, 300.0))
            return 0;
        // How long: usually 1 click, sometimes 2 or 3.
        float roll = random_rotation(rot_make_floats(0.0f, 1.0f));
        clicks = 3;
        if (!(walk_update(auto_clicker_rng_states_second, rot_make_floats(0.03f, 0.06f), 120.0) > roll))
            clicks = (walk_update(auto_clicker_rng_states_second, rot_make_floats(0.12f, 0.15f), 120.0) > roll) + 1;
    }
    auto_clicker_burst_clicks_left = clicks - 1;
    double extra = (double)gauss_random(auto_clicker_rng_states_third) * 0.01 + 0.02;
    *delay = (float)((double)*delay + extra);
    return 1;
}

// Burst chance bonus: higher while the view turns fast, and with no target within 5 blocks.
static float aim_bonus(Ctx *ctx) {
    float bonus = rot_delta_length(get_entity_rotation(ctx->jvm, ctx->player), auto_clicker_rotation_at_last_tick) / 20.0f + 0.0f;
    CombatTarget *target = (CombatTarget *)ctx->target;
    if (!target->ent || target->dist2 > 25.0)
        bonus = (float)((double)bonus + 0.08);
    return bonus;
}

// Seconds the button stays down after a press (the release takes the same time).
static float press_duration(Ctx *ctx) {
    float cps = auto_clicker_cps_setting;
    if (auto_clicker_randomize_setting == 1) {
        // Slow drifts of +-10% and quick wobbles of +-5%, then gaussian noise of 20-30%.
        float noise_size = walk_update(auto_clicker_random_walk_1, rot_make_floats(0.2f, 0.3f), 30.0);
        float slow_drift = walk_update(auto_clicker_random_walk_2, rot_make_floats(-0.1f, 0.1f), 120.0);
        float drift = walk_update(auto_clicker_random_walk_3, rot_make_floats(-0.1f, 0.1f), 30.0);
        float wobble = walk_update(auto_clicker_random_walk_4, rot_make_floats(-0.05f, 0.05f), 5.0);
        cps *= slow_drift + 1.0f + drift + wobble;
        cps *= gauss_random(auto_clicker_gauss_rng_state) * noise_size + 1.0f;
    }

    float delay;
    if (auto_clicker_pattern == 1) {
        if (auto_clicker_double_click_pending == 1) {
            delay = 0.014f;   // the second click of a double click
            auto_clicker_double_click_pending = 0;
        } else {
            delay = (float)(1.0 / (double)cps + -(double)0.014f);
            if (!auto_clicker_extra_burst_clicks(&delay, aim_bonus(ctx)))
                auto_clicker_double_click_pending = random_chance((float)walk_should_change(auto_clicker_random_walk_5, rot_make_floats(0.94f, 0.97f), 30.0));
        }
    } else {
        delay = 0.5f / cps;   // half a click down, half up
        auto_clicker_extra_burst_clicks(&delay, aim_bonus(ctx));
    }
    return delay;
}

// In an inventory: holding shift and dragging with the left button shift-clicks each slot.
static void inventory_fill(Ctx *ctx) {
    unsigned char *keys = (unsigned char *)ctx->key_state_table;
    unsigned char left_down = keys[1];
    double wait;
    if (left_down == 1 && auto_clicker_lmb_was_down_gui == 1) {
        bool shift_held = (keys[0x62] & 1) || (keys[0x63] & 1);
        if (auto_clicker_allow_inventory == 1 && shift_held) {
            void *jvm = ctx->jvm;
            void *screen = get_current_screen(jvm, ctx->minecraft);
            void *inventory = java_get_entity_player_inventory(jvm, ctx->player);
            void *slot = java_get_gui_container_the_slot(jvm, screen);
            if (slot && !java_get_inventory_player_item_stack(jvm, inventory)) {
                int slot_number = java_get_slot_slot_number(jvm, slot);
                if (slot_number != auto_clicker_last_inventory_slot_clicked) {
                    auto_clicker_last_inventory_slot_clicked = slot_number;
                    // Empty slots are clicked only now and then (like a human dragging over them).
                    if (java_call_slot_get_stack(jvm, slot) ||
                        !walk_should_change(auto_clicker_random_walk_6, rot_make_floats(0.6f, 0.8f), 10.0)) {
                        post_synthetic_click(1, 1);
                        if (auto_clicker_inventory_random_delay_setting != 1) {
                            wait = 0.001;
                        } else {
                            Range r = walk_should_change(auto_clicker_random_walk_7, rot_make_floats(0.04f, 0.08f), 10.0)
                                          ? rot_make_floats(0.15f, 0.25f) : rot_make_floats(0.06f, 0.075f);
                            wait = random_rotation(r);
                        }
                        auto_clicker_time_until_next_click_edge = wait + auto_clicker_time_until_next_click_edge;
                        return;
                    }
                }
            }
        }
    } else {
        auto_clicker_lmb_was_down_gui = left_down == 1;
    }
    wait = auto_clicker_inventory_random_delay_setting ? 0.05 : 0.001;
    auto_clicker_time_until_next_click_edge = wait + auto_clicker_time_until_next_click_edge;
}

// Every frame: send the next press or release when it is due.
extern "C" void auto_clicker_on_frame(TickHook *hook) {
    if (auto_clicker_attack_key != KEY_LEFT_MOUSE && auto_clicker_attack_key != KEY_RIGHT_MOUSE)
        return;   // attack is not on a mouse button: nothing to click
    Ctx *ctx = hook->ctx;

    // Count down by the frame time (scaled with the game speed), not below -0.2 s.
    double time_left = auto_clicker_time_until_next_click_edge - (double)java_get_timer_timer_speed(ctx->jvm, get_timer(ctx->jvm, ctx->minecraft)) * hook->dt;
    time_left = -0.2 > time_left ? -0.2 : time_left;
    auto_clicker_time_until_next_click_edge = time_left;
    if (!(0.0 >= time_left))
        return;

    // The attack button's state: bit 0 = down, bit 1 = held down by us.
    unsigned char *mouse = (unsigned char *)ctx->key_state_table;
    unsigned char down = mouse[auto_clicker_attack_bound_mouse + 1];
    unsigned char ours = (down & 2) >> 1;
    int button = auto_clicker_attack_bound_mouse ^ 1;
    double reset_to = 0.01;

    if (ctx->in_game == 0) {
        // In a menu: let go of our press, then the inventory feature.
        if (ours & 1)
            post_synthetic_click(button, (~down) & 1);
        if (get_current_screen(ctx->jvm, ctx->minecraft)) {
            inventory_fill(ctx);
            return;
        }
    } else {
        for (;;) {
            if (ctx->in_game == 0) {
                // (left the world during the loop)
                if (ours & 1)
                    post_synthetic_click(button, (~down) & 1);
                if (get_current_screen(ctx->jvm, ctx->minecraft)) {
                    inventory_fill(ctx);
                    return;
                }
                break;
            }
            if (!((down | ours) & 1))
                break;   // the player let go of the button

            // Reasons to stop clicking: release our press, if any.
            bool cancel = false;
            if (auto_clicker_hit_type_under_crosshair == 1 && auto_clicker_break_blocks_setting)
                cancel = true;
            else if (auto_clicker_require_weapon_value == 1 && !is_holding_accepted_weapon(ctx->jvm, ctx->player))
                cancel = true;
            else if (auto_clicker_pause_when_using_setting == 1)
                cancel = java_call_entity_player_is_using_item(ctx->jvm, ctx->player) || ((down & ours) & 1);
            else if ((down & ours) & 1) {
                post_synthetic_click(button, 0);   // release (the player is still holding it)
                reset_to = 0.0;
                break;
            }
            if (cancel) {
                reset_to = 0.0;
                if (ours & 1)
                    post_synthetic_click(button, (~down) & 1);
                break;
            }

            if (down & 1) {
                // Down now: release, after the same time it was down.
                post_synthetic_click(button, 0);
                auto_clicker_time_until_next_click_edge = auto_clicker_last_edge_delay + auto_clicker_time_until_next_click_edge;
            } else {
                // Up now: press, and decide how long it stays down.
                post_synthetic_click(button, 1);
                double delay = press_duration(ctx);
                auto_clicker_time_until_next_click_edge = auto_clicker_time_until_next_click_edge + delay;
                auto_clicker_last_edge_delay = delay;
            }
            down ^= 1;
            ours ^= 1;
            if (0.0 < auto_clicker_time_until_next_click_edge)
                return;
        }
    }

    auto_clicker_time_until_next_click_edge = reset_to;
    auto_clicker_last_inventory_slot_clicked = -1;
}
