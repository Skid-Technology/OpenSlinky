// 0x9000: Vec3 / AABB helpers (MS ABI: 24/48-byte structs travel by pointer, results via sret)
#include "slinky/engine.h"
struct Vec3 { double x, y, z; };
struct AABB { Vec3 min, max; };
extern "C" double unused_constant_1_62, unused_constant_neg_1_62_b;

extern "C" Vec3 vec3_make(double x, double y, double z) { return Vec3{x, y, z}; }
extern "C" Vec3 vec3_sub(Vec3 a, Vec3 b) { return Vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }
extern "C" Vec3 vec3_add(Vec3 a, Vec3 b) { return Vec3{b.x + a.x, b.y + a.y, b.z + a.z}; }
extern "C" Vec3 vec3_scale(Vec3 a, double s) { return Vec3{s * a.x, s * a.y, s * a.z}; }
extern "C" Vec3 vec3_lerp(Vec3 a, Vec3 b, double t) {
    return Vec3{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
extern "C" double vec3_distance_sq(Vec3 *a, Vec3 *b) {
    double dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
    return dz * dz + (dx * dx + dy * dy);
}
extern "C" double vec3_distance(Vec3 *a, Vec3 *b) {
    double ax = a->x, ay = a->y, az = a->z;
    double dx = ax - b->x, dy = ay - b->y, dz = az - b->z;
    return __builtin_sqrt(dz * dz + (dx * dx + dy * dy));
}
extern "C" Vec3 vec3_add_eye_height(Vec3 a) { return Vec3{a.x, a.y + unused_constant_1_62, a.z}; }
extern "C" Vec3 vec3_add_eye_height_sneak(Vec3 a) { return Vec3{a.x, a.y + unused_constant_neg_1_62_b, a.z}; }
extern "C" Vec3 vec3_reciprocal(Vec3 a) { return Vec3{1.0 / a.x, 1.0 / a.y, 1.0 / a.z}; }
extern "C" AABB aabb_make(Vec3 a, Vec3 b) { return AABB{a, b}; }
extern "C" AABB *aabb_expand(AABB *out, AABB *b, double d) {
    b->min.x -= d; b->min.y -= d; b->min.z -= d;
    b->max.x += d; b->max.y += d; b->max.z += d;
    *out = *b;
    return out;
}
extern "C" bool aabb_intersects(AABB *a, AABB *b) {
    return b->max.x > a->min.x && a->max.x > b->min.x && b->max.y > a->min.y &&
           a->max.y > b->min.y && b->max.z > a->min.z && a->max.z > b->min.z;
}
extern "C" bool aabb_contains_point(AABB *a, Vec3 *p) {
    return p->x > a->min.x && a->max.x > p->x && p->y > a->min.y && a->max.y > p->y &&
           p->z > a->min.z && a->max.z > p->z;
}

// ---- Rot (yaw, pitch) packed in one register ----
struct Rot { float yaw, pitch; };
FN(float, libc_hypotf_alt, (float a, float b));
FN(float, libc_hypotf, (float a, float b));
extern "C" double libc_atan2(double y, double x);
extern "C" double half_turn_degrees, pi_constant, offset_neg_90, minus_half_turn_degrees;

extern "C" Rot rot_make(float a, float b) { return Rot{a, b}; }
extern "C" Rot rot_add(Rot a, Rot b) { return Rot{b.yaw + a.yaw, b.pitch + a.pitch}; }
extern "C" Rot rot_sub(Rot a, Rot b) { return Rot{a.yaw - b.yaw, a.pitch - b.pitch}; }
extern "C" Rot rot_scale(Rot a, float s) { return Rot{a.yaw * s, a.pitch * s}; }
extern "C" Rot rot_lerp(Rot a, Rot b, float t) {
    float y = (b.yaw - a.yaw) * t + a.yaw;
    asm("" : "+x"(y));
    return Rot{y, (b.pitch - a.pitch) * t + a.pitch};
}
extern "C" float rot_delta_length(Rot a, Rot b) { return libc_hypotf_alt(a.yaw - b.yaw, a.pitch - b.pitch); }
extern "C" float rot_distance(Rot a, Rot b) { return libc_hypotf(b.pitch - a.pitch, b.yaw - a.yaw); }
extern "C" float rot_length(Rot a) { return libc_hypotf_alt(a.yaw, a.pitch); }
extern "C" Rot rot_look_at(Vec3 *from, Vec3 *to) {
    double dx = to->x - from->x;

    double dy = to->y - from->y;

    double dz = to->z - from->z;
    double hor = __builtin_sqrt(dx * dx + dz * dz);
    float yaw = (float)(libc_atan2(dz, dx) * half_turn_degrees / pi_constant + offset_neg_90);
    float pitch = (float)(libc_atan2(dy, hor) * minus_half_turn_degrees / pi_constant);
    return Rot{yaw, pitch};
}

// key code -> name (clang turns the switch into a relative string table)
extern "C" const char str_none[], str_mouse1[], str_mouse2[], str_mouse3[], str_mouse4[], str_mouse5[], str_backspace[], str_tab[], str_enter[], str_caps[], str_esc[], str_space[], str_prev[], str_next[], str_end[], str_home[], str_left[], mc_obf_name_up[], str_right[], str_down[], str_prntscr[], str_ins[], str_del[], str_numlock[], str_scroll[], str_lshift[], str_rshift[], str_lcontrol[], str_rcontrol[], str_lmenu[], str_rmenu[], str_colon[], str_plus[], str_comma[], str_minus[], str_period[], str_slash[], str_grave[], str_lbracket[], str_backslash[], str_rbracket[], str_quote[], text_number_0[], text_number_1[], text_number_2[], text_number_3[], text_number_4[], text_number_5[], text_number_6[], text_number_7[], text_number_8[], text_number_9[], mc_obf_name_a[], mc_obf_name_b[], mc_obf_name_c[], mc_obf_name_d[], mc_obf_name_e[], mc_obf_name_f[], mc_obf_name_g[], mc_obf_name_h[], mc_obf_name_i[], mc_obf_name_j[], mc_obf_name_k[], mc_obf_name_l[], mc_obf_name_m[], mc_obf_name_n[], mc_obf_name_o[], mc_obf_name_p[], mc_obf_name_q[], mc_obf_name_r[], mc_obf_name_s[], mc_obf_name_t[], mc_obf_name_u[], mc_obf_name_v[], mc_obf_name_w[], mc_obf_name_x[], mc_obf_name_y[], mc_obf_name_z[], str_numpad0[], str_numpad1[], str_numpad2[], str_numpad3[], str_numpad4[], str_numpad5[], str_numpad6[], str_numpad7[], str_numpad8[], str_numpad9[], mc_obf_name_f1[], mc_obf_name_f2[], mc_obf_name_f3[], mc_obf_name_f4[], mc_obf_name_f5[], mc_obf_name_f6[], mc_obf_name_f7[], mc_obf_name_f8[], mc_obf_name_f9[], text_f10[], text_f11[], text_f12[], text_f13[], text_f14[], text_f15[], text_f16[];
extern "C" const char *mouse_button_name(int v) {
    switch (v) {
    case 1: return str_mouse1;   // "MOUSE1"
    case 2: return str_mouse2;   // "MOUSE2"
    case 3: return str_mouse3;   // "MOUSE3"
    case 4: return str_mouse4;   // "MOUSE4"
    case 5: return str_mouse5;   // "MOUSE5"
    case 6: return text_number_0;   // "0"
    case 7: return text_number_1;   // "1"
    case 8: return text_number_2;   // "2"
    case 9: return text_number_3;   // "3"
    case 10: return text_number_4;   // "4"
    case 11: return text_number_5;   // "5"
    case 12: return text_number_6;   // "6"
    case 13: return text_number_7;   // "7"
    case 14: return text_number_8;   // "8"
    case 15: return text_number_9;   // "9"
    case 16: return str_numpad0;   // "NUMPAD0"
    case 17: return str_numpad1;   // "NUMPAD1"
    case 18: return str_numpad2;   // "NUMPAD2"
    case 19: return str_numpad3;   // "NUMPAD3"
    case 20: return str_numpad4;   // "NUMPAD4"
    case 21: return str_numpad5;   // "NUMPAD5"
    case 22: return str_numpad6;   // "NUMPAD6"
    case 23: return str_numpad7;   // "NUMPAD7"
    case 24: return str_numpad8;   // "NUMPAD8"
    case 25: return str_numpad9;   // "NUMPAD9"
    case 26: return mc_obf_name_a;   // "A"
    case 27: return mc_obf_name_b;   // "B"
    case 28: return mc_obf_name_c;   // "C"
    case 29: return mc_obf_name_d;   // "D"
    case 30: return mc_obf_name_e;   // "E"
    case 31: return mc_obf_name_f;   // "F"
    case 32: return mc_obf_name_g;   // "G"
    case 33: return mc_obf_name_h;   // "H"
    case 34: return mc_obf_name_i;   // "I"
    case 35: return mc_obf_name_j;   // "J"
    case 36: return mc_obf_name_k;   // "K"
    case 37: return mc_obf_name_l;   // "L"
    case 38: return mc_obf_name_m;   // "M"
    case 39: return mc_obf_name_n;   // "N"
    case 40: return mc_obf_name_o;   // "O"
    case 41: return mc_obf_name_p;   // "P"
    case 42: return mc_obf_name_q;   // "Q"
    case 43: return mc_obf_name_r;   // "R"
    case 44: return mc_obf_name_s;   // "S"
    case 45: return mc_obf_name_t;   // "T"
    case 46: return mc_obf_name_u;   // "U"
    case 47: return mc_obf_name_v;   // "V"
    case 48: return mc_obf_name_w;   // "W"
    case 49: return mc_obf_name_x;   // "X"
    case 50: return mc_obf_name_y;   // "Y"
    case 51: return mc_obf_name_z;   // "Z"
    case 52: return str_colon;   // "COLON"
    case 53: return str_plus;   // "PLUS"
    case 54: return str_comma;   // "COMMA"
    case 55: return str_minus;   // "MINUS"
    case 56: return str_period;   // "PERIOD"
    case 57: return str_slash;   // "SLASH"
    case 58: return str_grave;   // "GRAVE"
    case 59: return str_lbracket;   // "LBRACKET"
    case 60: return str_backslash;   // "BACKSLASH"
    case 61: return str_rbracket;   // "RBRACKET"
    case 62: return str_quote;   // "QUOTE"
    case 63: return str_left;   // "LEFT"
    case 64: return mc_obf_name_up;   // "UP"
    case 65: return str_right;   // "RIGHT"
    case 66: return str_down;   // "DOWN"
    case 67: return mc_obf_name_f1;   // "F1"
    case 68: return mc_obf_name_f2;   // "F2"
    case 69: return mc_obf_name_f3;   // "F3"
    case 70: return mc_obf_name_f4;   // "F4"
    case 71: return mc_obf_name_f5;   // "F5"
    case 72: return mc_obf_name_f6;   // "F6"
    case 73: return mc_obf_name_f7;   // "F7"
    case 74: return mc_obf_name_f8;   // "F8"
    case 75: return mc_obf_name_f9;   // "F9"
    case 76: return text_f10;   // "F10"
    case 77: return text_f11;   // "F11"
    case 78: return text_f12;   // "F12"
    case 79: return text_f13;   // "F13"
    case 80: return text_f14;   // "F14"
    case 81: return text_f15;   // "F15"
    case 82: return text_f16;   // "F16"
    case 83: return str_backspace;   // "BACKSPACE"
    case 84: return str_tab;   // "TAB"
    case 85: return str_enter;   // "ENTER"
    case 86: return str_caps;   // "CAPS"
    case 87: return str_esc;   // "ESC"
    case 88: return str_space;   // "SPACE"
    case 89: return str_prev;   // "PREV"
    case 90: return str_next;   // "NEXT"
    case 91: return str_end;   // "END"
    case 92: return str_home;   // "HOME"
    case 93: return str_prntscr;   // "PRNTSCR"
    case 94: return str_ins;   // "INS"
    case 95: return str_del;   // "DEL"
    case 96: return str_numlock;   // "NUMLOCK"
    case 97: return str_scroll;   // "SCROLL"
    case 98: return str_lshift;   // "LSHIFT"
    case 99: return str_rshift;   // "RSHIFT"
    case 100: return str_lcontrol;   // "LCONTROL"
    case 101: return str_rcontrol;   // "RCONTROL"
    case 102: return str_lmenu;   // "LMENU"
    case 103: return str_rmenu;   // "RMENU"
    default: return str_none;   // "NONE"
    }
}

// ---- AABB union / clamp / ray slab tests (double fmin/fmax on float slab values, like the C source) ----
extern "C" AABB aabb_union(Vec3 a, Vec3 b) {
    AABB r;
    r.min.z = __builtin_fmin(a.z, b.z);
    r.max.z = __builtin_fmax(a.z, b.z);
    r.min.x = __builtin_fmin(a.x, b.x); r.min.y = __builtin_fmin(a.y, b.y);
    r.max.x = __builtin_fmax(a.x, b.x); r.max.y = __builtin_fmax(a.y, b.y);
    return r;
}
extern "C" Vec3 aabb_clamp_point(AABB b, Vec3 p) {
    Vec3 r;
    r.x = p.x < b.min.x ? b.min.x : (b.max.x < p.x ? b.max.x : p.x);
    r.y = p.y < b.min.y ? b.min.y : (b.max.y < p.y ? b.max.y : p.y);
    r.z = p.z < b.min.z ? b.min.z : (b.max.z < p.z ? b.max.z : p.z);
    return r;
}
extern "C" const double positive_infinity, one_constant, positive_infinity_for_ray_test;
extern "C" const float threshold_1_in_vec_aabb, threshold_1_in_vec_aabb_a;
extern "C" bool aabb_ray_slab_test(const AABB *b, const Vec3 *o, const Vec3 *inv) {
    const double *mn = &b->min.x, *mx = &b->max.x, *po = &o->x, *pi = &inv->x;
    float tmin = 0.0f, tmax = (float)positive_infinity;
#pragma clang loop unroll(full)
    for (int i = 0; i < 3; i++) {
        float t1 = (float)((mn[i] - po[i]) * pi[i]);
        float t2 = (float)((mx[i] - po[i]) * pi[i]);
        tmin = (float)__builtin_fmin(__builtin_fmax(t1, tmin), __builtin_fmax(t2, tmin));
        tmax = (float)__builtin_fmax(__builtin_fmin(t1, tmax), __builtin_fmin(t2, tmax));
    }
    return tmin < tmax && tmin <= threshold_1_in_vec_aabb;
}
extern "C" bool aabb_ray_hit(const AABB *b, const Vec3 *o, const Vec3 *dir, Vec3 *hit) {
    double inv[3] = {one_constant / dir->x, one_constant / dir->y, one_constant / dir->z};
    const double *mn = &b->min.x, *mx = &b->max.x, *po = &o->x;
    float tmin = 0.0f, tmax = (float)positive_infinity_for_ray_test;
#pragma clang loop unroll(full)
    for (int i = 0; i < 3; i++) {
        float t1 = (float)((mn[i] - po[i]) * inv[i]);
        float t2 = (float)((mx[i] - po[i]) * inv[i]);
        tmin = (float)__builtin_fmin(__builtin_fmax(t1, tmin), __builtin_fmax(t2, tmin));
        tmax = (float)__builtin_fmax(__builtin_fmin(t1, tmax), __builtin_fmin(t2, tmax));
    }
    bool ok = !(tmax <= tmin) & !(threshold_1_in_vec_aabb_a < tmin);
    if (ok) {
        hit->x = o->x + dir->x * tmin;
        hit->y = o->y + dir->y * tmin;
        hit->z = o->z + dir->z * tmin;
    }
    return ok;
}

// ---- packet hook: GUI-chat commands first, then the packet event (0x17); returns "cancel" ----
#include "slinky/jids.h"
struct HCtx { Jvm *jvm; void *module_manager; void *profile_list; void *minecraft; void *world; void *player; void *key_state_table; void *target;
              unsigned char in_game; unsigned char padding_41[3]; int padding_44; };
extern "C" char java_natives_world_module_manager[], java_natives_frame_module_manager_target_state[];
extern "C" void *init_profile_list, *java_natives_world_key_state_table_one_byte;
extern "C" char *ptr_to_gui_module_descriptor;
FN(void *, get_minecraft, (Jvm *jvm));
FN(void *, get_local_player, (Jvm *jvm, void *minecraft));
FN(void *, get_world, (Jvm *jvm, void *minecraft));
FN(unsigned char, is_in_game, (Jvm *jvm, void *minecraft));
FN(void *, java_jni_is_instance_of_c01_packet_chat_message, (Jvm *jvm, void *pkt));
FN(void *, java_jni_is_instance_of_c14_packet_tab_complete, (Jvm *jvm, void *pkt));
FN(unsigned char, handle_chat_message, (HCtx *ctx, void *object));
FN(unsigned char, text_starts_with_bang, (HCtx *ctx, void *object));
struct PktEvent { long long type; Jvm *j; void *a; void *pkt; unsigned char cancel; char rest[0x27]; };
FN(void, dispatch_event, (void *mgr, PktEvent *e));
extern "C" unsigned char handle_chat_packet(Jvm *jvm, void *object, void *pkt) {
    void *minecraft = get_minecraft(jvm);
    void *player = get_local_player(jvm, minecraft);
    void *world = get_world(jvm, minecraft);
    HCtx ctx;
    ctx.jvm = jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = world; ctx.player = player;
    ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
    ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(jvm, minecraft);
    *(int *)&ctx.padding_41[0] = 0;
    ctx.padding_44 = 0;
    void *c01_packet_chat_message = java_jni_is_instance_of_c01_packet_chat_message(jvm, pkt);
    unsigned char r;
    if (c01_packet_chat_message) {
        r = handle_chat_message(&ctx, c01_packet_chat_message);
    } else {
        void *c14_packet_tab_complete = java_jni_is_instance_of_c14_packet_tab_complete(jvm, pkt);
        if (!c14_packet_tab_complete) goto send;
        r = text_starts_with_bang(&ctx, c14_packet_tab_complete);
    }
    if (r) return 1;
send:
    PktEvent e = {0x17, jvm, object, pkt, 0, {}};
    dispatch_event(java_natives_world_module_manager, &e);
    return e.cancel;
}

// ---- gap-scan functions ----
// client key code -> ASCII for text fields (a-z, 0-9, space, '.', '-'); 0 = not printable. Inverse of windows_key_to_client_key.
extern "C" char key_code_to_char(int key) {
    if ((unsigned)(key - 0x1a) <= 0x19) return (char)(key + 0x47);
    if ((unsigned)(key - 6) <= 9) return (char)(key + 0x2a);
    switch (key) {
    case 0x58: return ' ';
    case 0x38: return '.';
    case 0x37: return '-';
    default: return 0;
    }
}

// horizontal distance (*horiz) and absolute height difference (*vert) between two points
typedef long long v2l9 __attribute__((vector_size(16)));
typedef double v2d9 __attribute__((vector_size(16)));
extern "C" const v2l9 vec_aabb_fabs_mask;   // fabs mask
extern "C" double libc_hypot(double x, double z);   // hypot
extern "C" void vec3_horizontal_distance(const Vec3 *a, const Vec3 *b, double *horiz, double *vert) {
    double ax = a->x, az = a->z;

    *horiz = libc_hypot(ax - b->x, az - b->z);
    v2d9 d = {a->y - b->y, 0};
    *vert = ((v2d9)((v2l9)d & vec_aabb_fabs_mask))[0];
}

extern "C" Vec3 vec3_copy(Vec3 v) {   // normalize
    double len = __builtin_sqrt((v.x * v.x + v.y * v.y) + v.z * v.z);
    v.x /= len;
    v.y /= len;
    v.z /= len;
    asm("" : "+m"(v));   // the original normalises its by-value copy in memory and copies it out
    return v;
}
extern "C" double vec3_length(const Vec3 *v) { return __builtin_sqrt((v->x * v->x + v->y * v->y) + v->z * v->z); }

struct V2f9 { float x, y; };
extern "C" float vec2_length_sq(V2f9 v) { return v.x * v.x + v.y * v.y; }
