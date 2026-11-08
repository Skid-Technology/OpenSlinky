// Instant Stop - stop dead when you let go of the forward key.
//
// When forward is released on the ground the player normally slides a little.
// For one tick this module presses back (plus left or right if that cancels the slide
// better), then gives the keys back to the player.
//
// Callback (descriptor slot +0x148):
//   instant_stop_on_tick  every client tick
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };

// What the module is doing right now (kept between ticks).
enum InstantStopState {
    IDLE = 0,             // forward not held
    FORWARD_HELD = 1,     // forward held last tick, watch for it being released
    COUNTER_PRESSED = 2,  // counter keys pressed this tick, restore them next tick
};
extern "C" int instant_stop_state;

// ---- game / engine functions ----
FN(unsigned char, is_in_game, (void *jvm, void *object));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *k));                                  // key binding pressed (by the game)
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(unsigned char, java_get_entity_on_ground, (void *jvm, void *entity));
FN(Vec3, java_get_entity_motion_x, (void *jvm, void *entity));
FN(Rot, get_entity_rotation, (void *jvm, void *entity));
FN(float, java_get_entity_living_base_land_movement_factor, (void *jvm, void *entity_living_base));
FN(unsigned char, java_call_entity_is_sprinting, (void *jvm, void *entity));
FN(Vec3, horizontal_motion, (Rot rotation, float fwd, float strafe, float speed));     // motion one tick of input adds
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));

static double horizontal_speed_squared(Vec3 v) { return v.x * v.x + v.z * v.z; }

extern "C" void instant_stop_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    int next_state = IDLE;

    if (is_in_game(ctx->jvm, ctx->minecraft)) {
        void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
        void *forward_key = java_get_game_settings_forward_key(ctx->jvm, settings);
        void *back_key = java_get_game_settings_back_key(ctx->jvm, settings);
        void *left_key = java_get_game_settings_left_key(ctx->jvm, settings);
        void *right_key = java_get_game_settings_right_key(ctx->jvm, settings);

        int state = instant_stop_state;
        if (state == COUNTER_PRESSED) {
            // Last tick we pressed counter keys: put them back to what the player is really holding.
            java_set_key_binding_pressed(ctx->jvm, back_key, java_get_key_binding_key_code(ctx->jvm, back_key));
            java_set_key_binding_pressed(ctx->jvm, left_key, java_get_key_binding_key_code(ctx->jvm, left_key));
            java_set_key_binding_pressed(ctx->jvm, right_key, java_get_key_binding_key_code(ctx->jvm, right_key));
        } else if (state == FORWARD_HELD) {
            // Still holding forward, or in the air: keep waiting (state unchanged).
            if (is_key_down(ctx->jvm, forward_key))
                return;
            if (!java_get_entity_on_ground(ctx->jvm, ctx->player))
                return;

            // Forward was just released on the ground. Try the possible counter inputs and
            // pick the one that leaves the least speed.
            Vec3 motion = java_get_entity_motion_x(ctx->jvm, ctx->player);
            Rot rotation = get_entity_rotation(ctx->jvm, ctx->player);
            float speed = java_get_entity_living_base_land_movement_factor(ctx->jvm, ctx->player);
            if (java_call_entity_is_sprinting(ctx->jvm, ctx->player))
                speed = (float)((double)speed / 1.3);   // remove the sprint bonus

            enum { BACK_AND_RIGHT = -1, BACK_ONLY = 0, BACK_AND_LEFT = 1, NOTHING = 999 };

            // Doing nothing vs. back + right.
            double speed_nothing = horizontal_speed_squared(motion);
            double speed_back_right = horizontal_speed_squared(vec3_add(motion, horizontal_motion(rotation, -1.0f, -1.0f, speed)));
            int best_choice = speed_nothing > speed_back_right ? BACK_AND_RIGHT : NOTHING;
            double best_speed = speed_back_right < speed_nothing ? speed_back_right : speed_nothing;

            // Back only.
            double speed_back = horizontal_speed_squared(vec3_add(motion, horizontal_motion(rotation, -1.0f, 0.0f, speed)));
            if (best_speed > speed_back)
                best_choice = BACK_ONLY;
            best_speed = speed_back < best_speed ? speed_back : best_speed;

            // Back + left.
            double speed_back_left = horizontal_speed_squared(vec3_add(motion, horizontal_motion(rotation, -1.0f, 1.0f, speed)));
            bool left_is_best = best_speed > speed_back_left;
            if (left_is_best)
                best_choice = BACK_AND_LEFT;

            if (best_choice != NOTHING) {
                java_set_key_binding_pressed(ctx->jvm, back_key, 1);
                java_set_key_binding_pressed(ctx->jvm, left_key, left_is_best);
                java_set_key_binding_pressed(ctx->jvm, right_key, best_choice == BACK_AND_RIGHT);
                next_state = COUNTER_PRESSED;
            }
        } else {
            // Idle: start watching once forward is held.
            if (!is_key_down(ctx->jvm, forward_key))
                return;
            next_state = FORWARD_HELD;
        }
    }
    instant_stop_state = next_state;
}
