// 0x36000: distance comparisons (vec3_distance takes its two Vec3 by value = pointers to caller copies)
#include "slinky/engine.h"
struct Vec3 { double x, y, z; };
FN(double, vec3_distance, (Vec3 *a, Vec3 *b));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_other_player_mp_other_player_mpx, (void *jvm, void *entity_other_player_mp));
extern "C" double threshold_0_01_in_reach_distance_checks;
extern "C" double segment_distance(Vec3 *a, Vec3 *b, Vec3 *c) {
    Vec3 t1, t2;
    t1 = *a; t2 = *b;
    double d1 = vec3_distance(&t1, &t2);
    t1 = *a; t2 = *c;
    return vec3_distance(&t1, &t2) - d1;
}
extern "C" bool entity_reach_check(Ctx *ctx, void *ent, Vec3 *p) {
    Vec3 eye = java_get_entity_pos_x(ctx->jvm, ent);
    Vec3 prev = java_get_entity_other_player_mp_other_player_mpx(ctx->jvm, ent);
    Vec3 p1 = *p, e1 = eye, q1 = prev, p2 = *p;
    double d1 = vec3_distance(&p2, &e1);
    return vec3_distance(&p1, &q1) - d1 > threshold_0_01_in_reach_distance_checks;
}

// ---- smooth the tracked server position/yaw over 3 ticks after every change ----
extern "C" Vec3 backtrack_previous_current_server_position_in_reach_distance_checks, backtrack_previous_current_server_position, backtrack_previous_current_server_position_in_reach_distance_checks_a;
extern "C" float backtrack_yaws_in_reach_distance_checks, backtrack_yaws, backtrack_yaws_in_reach_distance_checks_a;
extern "C" int distance_compare_sample_count;
extern "C" const double one_in_reach_distance_checks;   // 1.0
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_scale, (Vec3 a, double s));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));
FN(float, wrap_angle, (float x));
extern "C" void backtrack_record_server_position(const Vec3 *pos, float yaw) {
    int n;
    if (__builtin_memcmp(pos, &backtrack_previous_current_server_position_in_reach_distance_checks, sizeof(Vec3)) == 0 && yaw == backtrack_yaws_in_reach_distance_checks) {
        n = distance_compare_sample_count;
    } else {
        distance_compare_sample_count = 3;
        n = 3;
    }
    backtrack_previous_current_server_position_in_reach_distance_checks = *pos;
    backtrack_previous_current_server_position = backtrack_previous_current_server_position_in_reach_distance_checks_a;
    backtrack_yaws_in_reach_distance_checks = yaw;
    backtrack_yaws = backtrack_yaws_in_reach_distance_checks_a;
    float ny;
    if (n > 0) {
        double f = one_in_reach_distance_checks / (double)n;
        Vec3 d = vec3_scale(vec3_sub(*pos, backtrack_previous_current_server_position_in_reach_distance_checks_a), f);
        backtrack_previous_current_server_position_in_reach_distance_checks_a = vec3_add(backtrack_previous_current_server_position_in_reach_distance_checks_a, d);
        ny = (float)((double)wrap_angle(backtrack_yaws_in_reach_distance_checks - backtrack_yaws_in_reach_distance_checks_a) * f) + backtrack_yaws_in_reach_distance_checks_a;
        distance_compare_sample_count--;
    } else {
        backtrack_previous_current_server_position_in_reach_distance_checks_a = *pos;
        ny = yaw;
    }
    backtrack_yaws_in_reach_distance_checks_a = ny;
}

// ---- track the target entity's position from move / teleport packets ----
// undo=true reverts the packet's delta (skipped when the packet is older than the last teleport)
struct PktCtx { void *jvm; void *pkt; void *padding_10; int tick; };
FN(Vec3, vec3_make, (double x, double y, double z));
FN(void *, java_jni_is_instance_of_s14_packet_entity, (void *jvm, void *pkt));
FN(int, java_get_s14_packet_entity_entity_id, (void *jvm, void *s14_packet_entity));
FN(signed char, java_jni_get_byte_field_s14_packet_entity_pos_x, (void *jvm, void *object));
FN(signed char, java_jni_get_byte_field_s14_packet_entity_pos_y, (void *jvm, void *object));
FN(signed char, java_jni_get_byte_field_s14_packet_entity_pos_z, (void *jvm, void *object));
FN(signed char, java_jni_get_byte_field_s14_packet_entity_yaw, (void *jvm, void *object));
FN(bool, java_get_s14_packet_entity_g, (void *jvm, void *s14_packet_entity));
FN(void *, java_jni_is_instance_of_s18_packet_entity_teleport, (void *jvm, void *pkt));
FN(int, java_get_s18_packet_entity_teleport_entity_id, (void *jvm, void *s18_packet_entity_teleport));
FN(int, java_get_s18_packet_entity_teleport_pos_x, (void *jvm, void *s18_packet_entity_teleport));
FN(int, java_get_s18_packet_entity_teleport_pos_y, (void *jvm, void *s18_packet_entity_teleport));
FN(int, java_get_s18_packet_entity_teleport_pos_z, (void *jvm, void *s18_packet_entity_teleport));
FN(signed char, java_jni_get_byte_field_s18_packet_entity_teleport_yaw, (void *jvm, void *object));
extern "C" int backtrack_target_entity_id, backtrack_stop_on_velocity_setting_field_0x2b;
extern "C" Vec3 backtrack_accumulated_offset;
extern "C" bool backtrack_stop_on_velocity_setting_field_0x1f, backtrack_stop_on_velocity_setting_field_0x27;
extern "C" float backtrack_stop_on_velocity_setting_field_0x23;
extern "C" const double factor_0_03125;   // 1/32
extern "C" const float full_turn_degrees_in_reach_distance_checks, factor_0_00390625;
extern "C" void check_entity_distance(PktCtx *c, bool undo) {
    if (undo && backtrack_stop_on_velocity_setting_field_0x2b > c->tick) return;
    if (void *s14_packet_entity = java_jni_is_instance_of_s14_packet_entity(c->jvm, c->pkt)) {
        if (java_get_s14_packet_entity_entity_id(c->jvm, s14_packet_entity) != backtrack_target_entity_id) return;
        double x = (double)java_jni_get_byte_field_s14_packet_entity_pos_x(c->jvm, s14_packet_entity) * factor_0_03125;
        double y = (double)java_jni_get_byte_field_s14_packet_entity_pos_y(c->jvm, s14_packet_entity) * factor_0_03125;
        double z = (double)java_jni_get_byte_field_s14_packet_entity_pos_z(c->jvm, s14_packet_entity) * factor_0_03125;
        if (undo) {
            backtrack_accumulated_offset = vec3_sub(backtrack_accumulated_offset, vec3_make(x, y, z));
            return;
        }
        backtrack_accumulated_offset = vec3_add(backtrack_accumulated_offset, vec3_make(x, y, z));
        if (!java_get_s14_packet_entity_g(c->jvm, s14_packet_entity)) return;
        backtrack_stop_on_velocity_setting_field_0x23 = (float)java_jni_get_byte_field_s14_packet_entity_yaw(c->jvm, s14_packet_entity) * full_turn_degrees_in_reach_distance_checks * factor_0_00390625;
        backtrack_stop_on_velocity_setting_field_0x27 = 1;
        return;
    }
    void *s18_packet_entity_teleport = java_jni_is_instance_of_s18_packet_entity_teleport(c->jvm, c->pkt);
    if (!s18_packet_entity_teleport) return;
    if (java_get_s18_packet_entity_teleport_entity_id(c->jvm, s18_packet_entity_teleport) != backtrack_target_entity_id) return;
    double x = (double)java_get_s18_packet_entity_teleport_pos_x(c->jvm, s18_packet_entity_teleport) * factor_0_03125;
    double y = (double)java_get_s18_packet_entity_teleport_pos_y(c->jvm, s18_packet_entity_teleport) * factor_0_03125;
    double z = (double)java_get_s18_packet_entity_teleport_pos_z(c->jvm, s18_packet_entity_teleport) * factor_0_03125;
    if (undo) {
        backtrack_accumulated_offset = vec3_sub(backtrack_accumulated_offset, vec3_make(x, y, z));
        backtrack_stop_on_velocity_setting_field_0x1f = 0;
        return;
    }
    backtrack_accumulated_offset = vec3_make(x, y, z);
    backtrack_stop_on_velocity_setting_field_0x1f = 1;
    backtrack_stop_on_velocity_setting_field_0x23 = (float)java_jni_get_byte_field_s18_packet_entity_teleport_yaw(c->jvm, s18_packet_entity_teleport) * full_turn_degrees_in_reach_distance_checks * factor_0_00390625;
    backtrack_stop_on_velocity_setting_field_0x27 = 1;
    backtrack_stop_on_velocity_setting_field_0x2b = c->tick;
}
