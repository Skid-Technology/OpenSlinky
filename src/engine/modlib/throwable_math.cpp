// 0x45000: throwable start position and velocity (vanilla EntityThrowable maths)
#include "slinky/engine.h"
struct Vec3 { double x, y, z; };
struct Rot { float yaw, pitch; };
FN(float, libc_sinf, (float x));   // sinf
FN(float, libc_cosf, (float x));   // cosf
FN(float, lerp_unclamped_float, (float a, float b, float t));
FN(Vec3, vec3_scale, (Vec3 v, double k));
FN(Vec3, vec3_add_eye_height, (Vec3 v));
FN(unsigned char, java_variant, (void *jvm));
FN(unsigned char, is_sneaking, (void *jvm, void *object));
typedef int v4i __attribute__((vector_size(16)));
typedef float v4f __attribute__((vector_size(16)));
extern "C" v4i throwable_math_sign_mask;   // sign mask
extern "C" const double degrees_to_radians, sneak_eye_height_drop, factor_0_16, tenth_in_throwable_math;
extern "C" float trajectories_launch_power, trajectories_launch_power_second;
static inline __attribute__((always_inline)) float neg(float x) {
    v4f v = {x, 0, 0, 0};
    return ((v4f)((v4i)v ^ throwable_math_sign_mask))[0];
}
extern "C" void throwable_velocity(Ctx *ctx, Vec3 *pos, Rot rotation, float t, Vec3 *outPos, Vec3 *outVel) {
    double k = degrees_to_radians;
    float yaw = (float)((double)rotation.yaw * k);
    float cy = libc_cosf(yaw);
    float pitch = (float)((double)rotation.pitch * k);
    float cp = libc_cosf(pitch);
    float sy = libc_sinf(yaw);
    float sp = libc_sinf(pitch);
    Vec3 dir = {(double)(neg(sy) * cp), (double)neg(sp), (double)(cp * cy)};
    Vec3 vel = vec3_scale(dir, (double)lerp_unclamped_float(trajectories_launch_power, trajectories_launch_power_second, t));
    if (!java_variant(ctx->jvm)) {
        *pos = vec3_add_eye_height(*pos);
        if (is_sneaking(ctx->jvm, ctx->player)) pos->y += sneak_eye_height_drop;
    }
    pos->x -= (double)cy * factor_0_16;
    pos->y += tenth_in_throwable_math;
    pos->z -= (double)sy * factor_0_16;
    *outPos = *pos;
    *outVel = vel;
}
