// 0xa000: Rot / Rect / float helpers, random numbers
#include "slinky/engine.h"
struct Rot { float yaw, pitch; };
struct Rect { float x, y, w, h; };
FN(float, libc_roundf, (float x));
FN(float, libc_fmodf, (float x, float y));
FN(double, time_now, ());
extern "C" float full_turn_degrees, half_turn_degrees_b, offset_neg_360, threshold_neg_180, threshold_1_in_math_helpers_a, three_constant;
extern "C" double current_time_seconds;

extern "C" __attribute__((noinline)) Rot rot_make_floats(float a, float b) { return Rot{a, b}; }
extern "C" Rect make_rect(float x, float y, float w, float h) { return Rect{x, y, w, h}; }
extern "C" bool point_in_rect(Rect *r, Rot p) {
    return p.yaw >= r->x && p.pitch >= r->y && r->x + r->w > p.yaw && r->y + r->h > p.pitch;
}
extern "C" bool rects_intersect(Rect *a, Rect *b) {
    if (a->x > b->x + b->w || b->x > a->x + a->w) return false;
    if (a->y > b->y + b->h) return false;
    return !(b->y > a->y + a->h);
}
typedef float v2f __attribute__((ext_vector_type(2)));
extern "C" Rect inset_rect(Rect r, float d) {
    v2f pos = {r.x, r.y}, size = {r.w, r.h};
    v2f dd = {d, d};
    float d2 = d + d;
    v2f d22 = {d2, d2};
    pos += dd;
    size -= d22;
    return Rect{pos.x, pos.y, size.x, size.y};
}
extern "C" Rect round_rect(Rect *r) {
    float x0 = r->x, y0 = r->y;
    float x = libc_roundf(x0), y = libc_roundf(y0), w = libc_roundf(r->w), h = libc_roundf(r->h);
    return Rect{x, y, w, h};
}
extern "C" Rot offset_rect(Rot *p) { return *p; }
extern "C" float limit_rotation_delta(float x, float a, float b) { return (b < x) ? x : (a < b ? a : b); }
extern "C" float wrap_angle(float x) {
    float r = libc_fmodf(x, full_turn_degrees);
    if (r >= half_turn_degrees_b) return r + offset_neg_360;
    if (threshold_neg_180 > r) {  r += full_turn_degrees; }
    return r;
}
extern "C" float lerp_clamped_float(float a, float b, float t) {
    float m = threshold_1_in_math_helpers_a < t ? threshold_1_in_math_helpers_a : t;
    return a + m * (b - a);
}
extern "C" float ease_curve(float t) { float sq = t * t; float d = t + t; return (three_constant - d) * sq; }
extern "C" float lerp_unclamped_float(float a, float b, float t) { return a + (b - a) * t; }
extern "C" double lerp_double(double a, double b, double t) { return a + (b - a) * t; }
extern "C" int chunk_coordinate(int x) { return x >> 4; }
extern "C" int block_coordinate(int x) { return x << 4; }
extern "C" void update_frame_time() { current_time_seconds = time_now(); }

// ---- xoshiro128+ random numbers ----
extern "C" unsigned init_table[4];
extern "C" const float float_epsilon_unused_a, one_in_math_helpers_b, float_epsilon_unused_b, random_range_bound_1_in_math_helpers_c, random_walk_range_epsilon_a, random_range_bound_1_in_math_helpers_d, random_walk_range_epsilon_b;
extern "C" const double threshold_0_2, threshold_0_2_in_math_helpers;
typedef int v4i __attribute__((vector_size(16)));
typedef float v4f __attribute__((vector_size(16)));
extern "C" const v4i math_helpers_fabs_masks, math_helpers_fabs_masks_second;   // fabs masks
static inline __attribute__((always_inline)) unsigned rng_next() {
    unsigned *s = init_table;
    unsigned result = s[0] + s[3];
    unsigned t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = (s[3] << 11) | (s[3] >> 21);
    return result;
}
static inline __attribute__((always_inline)) float rng_range(Rot rotation, float scale) {
    float d = rotation.pitch - rotation.yaw;
    return (float)(rng_next() >> 8) * scale * d + rotation.yaw;
}
static inline __attribute__((always_inline)) float fabs_m(float x, v4i m) {
    v4f v = {x, 0, 0, 0};
    return ((v4f)((v4i)v & m))[0];
}
extern "C" float random_rotation(Rot rotation) { return rng_range(rotation, float_epsilon_unused_a); }
extern "C" bool random_chance(float p) { return p > rng_range(rot_make_floats(0.0f, one_in_math_helpers_b), float_epsilon_unused_b); }

struct Walk { double t0; float a, b; };
extern "C" __attribute__((noinline)) void walk_randomize(Walk *w, bool both) {
    do {
        if (both) w->a = rng_range(rot_make_floats(0.0f, random_range_bound_1_in_math_helpers_d), random_walk_range_epsilon_b);
        w->b = rng_range(rot_make_floats(0.0f, random_range_bound_1_in_math_helpers_d), random_walk_range_epsilon_b);
    } while (threshold_0_2_in_math_helpers > (double)fabs_m(w->a - w->b, math_helpers_fabs_masks_second));
}
// smooth random value in [r.yaw, r.pitch], re-targeted every `period` seconds
extern "C" __attribute__((noinline)) float walk_update(Walk *w, Rot rotation, double period) {
    if (current_time_seconds > w->t0 + period) {
        w->t0 = current_time_seconds;
        if (w->a == 0.0f && w->b == 0.0f) {
            walk_randomize(w, true);
        } else {
            w->a = w->b;
            do {
                w->b = rng_range(rot_make_floats(0.0f, random_range_bound_1_in_math_helpers_c), random_walk_range_epsilon_a);
            } while (threshold_0_2 > (double)fabs_m(w->a - w->b, math_helpers_fabs_masks));
        }
    }
    double t = (current_time_seconds - w->t0) / period;
    double u = (double)(w->b - w->a) * t + (double)w->a;
    return (float)((double)(rotation.pitch - rotation.yaw) * u + (double)rotation.yaw);
}
extern "C" const float random_range_bound_1_in_math_helpers_e, random_walk_range_epsilon_c, random_range_bound_1_in_math_helpers_f, random_walk_range_epsilon_d;
extern "C" const double threshold_0_2_in_math_helpers_a;
extern "C" const v4i abs_mask;
// same walk driven by an accumulated time step instead of the clock
extern "C" float walk_update_timed(Walk *w, Rot rotation, double period, double dt) {
    w->t0 += dt;
    if (w->t0 > period) {
        w->t0 = 0.0;
        if (w->a == 0.0f && w->b == 0.0f) {
            walk_randomize(w, true);
        } else {
            w->a = w->b;
            do {
                w->b = rng_range(rot_make_floats(0.0f, random_range_bound_1_in_math_helpers_e), random_walk_range_epsilon_c);
            } while (threshold_0_2_in_math_helpers_a > (double)fabs_m(w->a - w->b, abs_mask));
        }
    }
    double t = w->t0 / period;
    double u = (double)(w->b - w->a) * t + (double)w->a;
    return (float)((double)(rotation.pitch - rotation.yaw) * u + (double)rotation.yaw);
}
extern "C" bool walk_should_change(Walk *w, Rot rotation, double period) {
    float x = rng_range(rot_make_floats(0.0f, random_range_bound_1_in_math_helpers_f), random_walk_range_epsilon_d);
    return walk_update(w, rotation, period) > x;
}

// Gaussian random number (Marsaglia polar method, second value cached)
FN(float, libc_hypotf_alt, (float a, float b));
FN(float, libc_logf, (float x));    // logf
FN(float, libc_sqrtf, (float x));    // sqrtf (errno path)
extern "C" const float factor_5_96046eneg_08_in_math_helpers_e, offset_neg_1, threshold_1_in_math_helpers, minus_two;
struct Gauss { float spare; bool has; };
extern "C" float gauss_random(Gauss *g) {
    if (g->has) {
        g->has = false;
        return g->spare;
    }
    float u, v, s;
    do {
        u = (float)(rng_next() >> 8) * factor_5_96046eneg_08_in_math_helpers_e;
        u = u + u + offset_neg_1;
        v = (float)(rng_next() >> 8) * factor_5_96046eneg_08_in_math_helpers_e;
        v = v + v + offset_neg_1;
        s = libc_hypotf_alt(u, v);
    } while (s >= threshold_1_in_math_helpers || s == 0.0f);
    float x = libc_logf(s) * minus_two / s;
    float m = x >= 0.0f ? __builtin_elementwise_sqrt(x) : libc_sqrtf(x);
    g->spare = v * m;
    g->has = true;
    return u * m;
}

// define (or find) a class in `loader` and register its native methods
#include "slinky/jnienv.h"
FN(jclass, find_class, (jobject loader, JNIEnv *env, const char *name));
FN(jobject, class_loader_ref, (jobject loader));
extern "C" int define_class(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len,
                       const JNINativeMethod *methods, jclass *out) {
    jclass cls = find_class(loader, env, name);
    if (!cls) {
        (*env)->ExceptionClear(env);
        auto define = (*env)->DefineClass;

        cls = define(env, name, class_loader_ref(loader), buf, len);
        if (!cls) return 0x7d6;
    }
    if ((*env)->RegisterNatives(env, cls, methods, 1) != 0) return 0x7d1;
    *out = cls;
    return 0;
}
extern "C" int define_class_with_natives(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len,
                       const JNINativeMethod *methods, jint n, jclass *out) {
    jclass cls = find_class(loader, env, name);
    if (!cls) {
        (*env)->ExceptionClear(env);
        auto define = (*env)->DefineClass;

        cls = define(env, name, class_loader_ref(loader), buf, len);
        if (!cls) return 0x7d6;
    }
    if ((*env)->RegisterNatives(env, cls, methods, n) != 0) return 0x7d1;
    *out = cls;
    return 0;
}

// one xoshiro128+ step of the shared generator (out-of-line copy of rng_next)
extern "C" unsigned xoshiro128_step_shared_generator() { return rng_next(); }

// ---- JavaVM lookup: JNI_GetCreatedJavaVMs from the already loaded "jvm" module (one VM) ----
// 0x3e8: no jvm module, 0x3e9: no such export, 0x7d2: the call failed
extern "C" const char str_jvm[], str_jni_getcreatedjavavms[];   // "jvm", "JNI_GetCreatedJavaVMs"
FN(void *, kernel32_lookups, (const char *module));
FN(void *, get_proc_address, (void *module, const char *name));
typedef int (*GetCreatedJavaVMs)(void **vms, int n, int *count);
extern "C" int get_java_vm(void **vm) {
    int err;
    if (!vm) return 0x7d2;
    *vm = nullptr;
    void *jvm = kernel32_lookups(str_jvm);
    if (!jvm) {
        err = 0x3e8;
    } else {
        GetCreatedJavaVMs get = (GetCreatedJavaVMs)get_proc_address(jvm, str_jni_getcreatedjavavms);
        if (!get) {
            err = 0x3e9;
        } else {
            int count = 0;
            void *created = nullptr;
            int result = get(&created, 1, &count);
            if (result == 0 && count >= 1 && created) {
                *vm = created;
                err = 0;
            } else {
                err = 0x7d2;   // call failed or no VM has been created yet
            }
        }
    }
    SHARED_EPILOG();
    return err;
}
