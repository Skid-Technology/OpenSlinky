// Reads the four movement keys into the timer_range strafe / forward input axes.
#include "slinky/engine.h"
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(unsigned char, is_key_down, (void *jvm, void *object));
extern "C" float one_in_timer_range_movement_axes, minus_one_in_timer_range_movement_axes, timer_range_last_forward_input, timer_range_last_strafe_input;
extern "C" void timer_range_read_movement_axes(void *jvm, void *ent) {
    void *game_settings_key_bindings = java_get_game_settings_forward_key(jvm, ent);
    void *key_binding = java_get_game_settings_back_key(jvm, ent);
    void *i2 = java_get_game_settings_left_key(jvm, ent);
    void *i3 = java_get_game_settings_right_key(jvm, ent);
    float c1 = one_in_timer_range_movement_axes;
    float x = c1;
    if (!is_key_down(jvm, game_settings_key_bindings)) x = 0.0f;
    float c2 = minus_one_in_timer_range_movement_axes;
    if (is_key_down(jvm, key_binding)) x += c2;
    float y = c1;
    if (!is_key_down(jvm, i2)) y = 0.0f;
    if (is_key_down(jvm, i3)) y += c2;
    timer_range_last_forward_input = x;
    timer_range_last_strafe_input = y;
}
