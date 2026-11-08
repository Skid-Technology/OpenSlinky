// 0x4a000: yaw-difference test
#include "slinky/engine.h"
struct Rot { float yaw, pitch; };
typedef float v4f __attribute__((vector_size(16)));
typedef int v4i __attribute__((vector_size(16)));
extern "C" v4i yaw_diff_fabs_mask;   // fabs mask
extern "C" float threshold_100_in_yaw_diff;
FN(double, java_get_entity_other_player_mp_other_player_mpyaw, (void *jvm, void *entity_other_player_mp));
FN(float, wrap_angle, (float x));
extern "C" bool yaw_diff_check(void **c, Rot rotation, void *ent) {
    float d = wrap_angle((float)java_get_entity_other_player_mp_other_player_mpyaw(*c, ent) - rotation.yaw);
    v4f v = {d, 0, 0, 0};
    v = (v4f)((v4i)v & yaw_diff_fabs_mask);
    return threshold_100_in_yaw_diff > v[0];
}
