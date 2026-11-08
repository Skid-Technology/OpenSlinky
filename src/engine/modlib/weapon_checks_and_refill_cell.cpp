// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
FN(unsigned char, is_held_item_accepted_weapon, (void *jvm, void *item, unsigned *flags));
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));
FN(void *, get_current_screen, (void *jvm, void *object));
FN(void, keyboard_send_key, (void *jvm, void *screen, int x, int y));
extern "C" char refill_module_descriptor[];
extern "C" unsigned char is_holding_accepted_weapon(void *jvm, void *object) { return is_held_item_accepted_weapon(jvm, object, 0); }
extern "C" void refill_disable_and_close_screen(Ctx *ctx) {
    queue_module_toggle(ctx->module_manager, refill_module_descriptor, 0);
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (screen) keyboard_send_key(ctx->jvm, screen, 0, 1);
}

// pick the target cell (of the 9x3 grid) closest to `cell`, scored by distance and yaw difference.
// type-2 cells always count, type-0 cells only when a random check passes; the best type-0 cell or the
// runner-up type-2 cell can win instead after a random walk check. *out = score, refill_cell_angle = angle.
struct Rot { float yaw, pitch; };
struct Walk { double t0; float a, b; };
FN(Rot, rot_make, (float a, float b));
FN(float, rot_delta_length, (Rot a, Rot b));
FN(float, rot_distance, (Rot a, Rot b));
FN(float, wrap_angle, (float a));
FN(bool, random_chance, (float p));
FN(Rot, rot_make_floats, (float a, float b));
FN(bool, walk_should_change, (Walk *w, Rot rotation, double period));
extern "C" unsigned char refill_slot_classes[27];
extern "C" Walk weapon_checks_and_refill_cell_random_walk_1, weapon_checks_and_refill_cell_random_walk_2;
extern "C" float refill_cell_angle;
extern "C" const float largest_float, chance_0_5_in_weapon_checks_and_refill_cell, factor_1_2_in_weapon_checks_and_refill_cell, random_range_bound_0_05_in_weapon_checks_and_refill_cell, random_range_bound_0_1_in_weapon_checks_and_refill_cell;
typedef int v4i __attribute__((vector_size(16)));
typedef float v4f __attribute__((vector_size(16)));
extern "C" const v4i float_abs_mask_v4i;
extern "C" const double walk_period_20_in_weapon_checks_and_refill_cell, threshold_1_5_in_weapon_checks_and_refill_cell, walk_period_10_in_weapon_checks_and_refill_cell;
extern "C" int refill_pick_inventory_cell(int cell, float yaw, float *out) {
    Rot cur = rot_make((float)(cell % 9), (float)(cell / 9));
    float best2 = largest_float, ang2 = 0, prev2 = largest_float, prevAng2 = 0;
    int idx2 = -1, prevIdx2 = -1;
    float best0 = largest_float, ang0 = 0;
    int idx0 = -1;
    for (unsigned long long i = 0; i < 27; i++) {
        unsigned char t = refill_slot_classes[i];
        if (t != 2) {
            if (t != 0) continue;
            if (random_chance(chance_0_5_in_weapon_checks_and_refill_cell)) continue;
        }
        unsigned char b = (unsigned char)i;
        Rot rotation = rot_make((float)(signed char)(b % 9), (float)(b / 9));
        float d = rot_delta_length(cur, rotation);
        float a = rot_distance(cur, rotation);
        float s = d * factor_1_2_in_weapon_checks_and_refill_cell + ((v4f)((v4i)(v4f){wrap_angle(a - yaw), 0, 0, 0} & float_abs_mask_v4i))[0] * chance_0_5_in_weapon_checks_and_refill_cell;
        if (t) {
            if (!(s > best2)) { prev2 = best2; prevIdx2 = idx2; prevAng2 = ang2; best2 = s; idx2 = (int)i; ang2 = a; }
        } else if (!(s > best0)) { best0 = s; idx0 = (int)i; ang0 = a; }
    }
    float rs = best2, ra = ang2;
    int ri = idx2;
    if (best2 > best0 && walk_should_change(&weapon_checks_and_refill_cell_random_walk_1, rot_make_floats(random_range_bound_0_05_in_weapon_checks_and_refill_cell, random_range_bound_0_1_in_weapon_checks_and_refill_cell), walk_period_20_in_weapon_checks_and_refill_cell)) {
        rs = best0; ra = ang0; ri = idx0;
    } else if (prevIdx2 != -1 && (double)best2 * threshold_1_5_in_weapon_checks_and_refill_cell > (double)prev2 &&
               walk_should_change(&weapon_checks_and_refill_cell_random_walk_2, rot_make_floats(random_range_bound_0_05_in_weapon_checks_and_refill_cell, random_range_bound_0_1_in_weapon_checks_and_refill_cell), walk_period_10_in_weapon_checks_and_refill_cell)) {
        rs = prev2; ra = prevAng2; ri = prevIdx2;
    }
    *out = rs;
    refill_cell_angle = ra;
    return ri;
}
