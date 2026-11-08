// JNI: entity rotation and angle accessors.
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"
struct Rot { float yaw, pitch; };
struct Vec3 { double x, y, z; };
FN(Vec3, vec3_make, (double x, double y, double z));
FN(Rot, rot_make, (float yaw, float pitch));
FN(float, limit_rotation_delta, (float lo, float hi, float v));
FN(float, libc_sinf, (float x));   // sinf
FN(float, libc_cosf, (float x));   // cosf
typedef int v4i __attribute__((vector_size(16), aligned(16)));
typedef float v4f __attribute__((vector_size(16), aligned(16)));
extern "C" const float clamp_bound_neg_90, clamp_bound_90, clamp_bound_neg_90_in_jni_entity_rotation, clamp_bound_90_in_jni_entity_rotation, twenty_in_jni_entity_rotation, twenty_in_jni_entity_rotation_a;
extern "C" const double neg_pi, half_turn_degrees_in_jni_entity_rotation, one_in_jni_entity_rotation;
extern "C" const v4i jni_entity_rotation_sign_mask;   // sign mask

extern "C" bool java_variant(Jvm *jvm) { return jvm->ids->variant == 0; }

// rotationYaw/rotationPitch packed into a Rot
extern "C" Rot get_entity_rotation(Jvm *jvm, jobject object) {
    if (!object) return Rot{0.0f, 0.0f};
    float y = JCALLN(jvm, GetFloatField, object, ids->Entity_rotationYaw);
    float pitch = JCALLN(jvm, GetFloatField, object, ids->Entity_rotationPitch);
    return rot_make(y, pitch);
}
extern "C" Rot get_entity_prev_rotation(Jvm *jvm, jobject object) {
    if (!object) return Rot{0.0f, 0.0f};
    float y = JCALLN(jvm, GetFloatField, object, ids->Entity_prevRotationYaw);
    float pitch = JCALLN(jvm, GetFloatField, object, ids->Entity_prevRotationPitch);
    return rot_make(y, pitch);
}
// set yaw, pitch clamped to [lo, hi]
extern "C" void set_entity_rotation(Jvm *jvm, jobject object, Rot rotation) {
    if (!object) return;
    JCALLN(jvm, SetFloatField, object, ids->Entity_rotationYaw, rotation.yaw);
    JIds *ids = jvm->ids; JNIEnv *env = jvm->env;
    auto f = (*env)->SetFloatField; jfieldID id = ids->Entity_rotationPitch;

    f(env, object, id, limit_rotation_delta(clamp_bound_neg_90, clamp_bound_90, rotation.pitch));
}
extern "C" void set_entity_prev_rotation(Jvm *jvm, jobject object, Rot rotation) {
    if (!object) return;
    JCALLN(jvm, SetFloatField, object, ids->Entity_prevRotationYaw, rotation.yaw);
    JIds *ids = jvm->ids; JNIEnv *env = jvm->env;
    auto f = (*env)->SetFloatField; jfieldID id = ids->Entity_prevRotationPitch;

    f(env, object, id, limit_rotation_delta(clamp_bound_neg_90_in_jni_entity_rotation, clamp_bound_90_in_jni_entity_rotation, rotation.pitch));
}
// look vector from the entity rotation
extern "C" Vec3 java_get_entity_rotation_pitch(Jvm *jvm, jobject entity) {
    float y = JCALLN(jvm, GetFloatField, entity, ids->Entity_rotationYaw);
    float pitch = JCALLN(jvm, GetFloatField, entity, ids->Entity_rotationPitch);
    Rot rotation = rot_make(y, pitch);
    float a = (float)((double)rotation.yaw * neg_pi / half_turn_degrees_in_jni_entity_rotation + neg_pi);
    float ca = libc_cosf(a);
    float sa = libc_sinf(a);
    float b = (float)((double)rotation.pitch * neg_pi / half_turn_degrees_in_jni_entity_rotation);
    float cb = ((v4f)((v4i)(v4f){libc_cosf(b)} ^ jni_entity_rotation_sign_mask))[0];
    float sb = libc_sinf(b);
    float x = sa * cb;
    Vec3 v = vec3_make(x, sb, cb * ca);

    return v;
}
extern "C" void java_set_entity_motion_x(Jvm *jvm, jobject entity, Vec3 value) {
    JCALLN(jvm, SetDoubleField, entity, ids->Entity_motionX, value.x);
    JCALLN(jvm, SetDoubleField, entity, ids->Entity_motionY, value.y);
    JCALL(jvm, SetDoubleField, entity, ids->Entity_motionZ, value.z);
}
extern "C" void set_motion(Jvm *jvm, jobject object, float x, float y, float z) {
    JNI(jvm)->CallVoidMethod(jvm->env, object, jvm->ids->Entity_moveFlying, (double)x, (double)y, (double)z);
}
extern "C" jobject java_call_entity_living_base_get_position(Jvm *jvm, jobject entity_living_base) {
    return JCALL1(jvm, CallObjectMethod, entity_living_base, ids->EntityLivingBase_getPosition, one_in_jni_entity_rotation);
}
static inline __attribute__((always_inline)) bool isnan_bits(float f) {
    return (__builtin_bit_cast(unsigned, f) & 0x7fffffff) >= 0x7f800001;
}
extern "C" float java_call_entity_living_base_get_health(Jvm *jvm, jobject entity_living_base) {
    float f = JCALLN(jvm, CallFloatMethod, entity_living_base, ids->EntityLivingBase_getHealth);
    return isnan_bits(f) ? twenty_in_jni_entity_rotation : f;
}
extern "C" float java_call_entity_living_base_get_max_health(Jvm *jvm, jobject entity_living_base) {
    float f = JCALLN(jvm, CallFloatMethod, entity_living_base, ids->EntityLivingBase_getMaxHealth);
    return isnan_bits(f) ? twenty_in_jni_entity_rotation_a : f;
}
extern "C" float java_call_entity_player_get_absorption_amount(Jvm *jvm, jobject entity_player) {
    float f = JCALLN(jvm, CallFloatMethod, entity_player, ids->EntityPlayer_getAbsorptionAmount);
    if (__builtin_expect(isnan_bits(f), 0)) f = 0.0f;
    return f;
}

extern "C" jobject java_get_entity_bounding_box(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetObjectField, entity, ids->Entity_boundingBox);
}

extern "C" void java_call_entity_set_sprinting(Jvm *jvm, void *entity, unsigned char value) {
    JCALL1(jvm, CallVoidMethod, entity, ids->Entity_setSprinting, value);
}

extern "C" jint java_get_entity_entity_id(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetIntField, entity, ids->Entity_entityId);
}

extern "C" jobject java_get_entity_uuid(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetObjectField, entity, ids->Entity_uniqueId);
}

extern "C" jint java_get_entity_ticks_existed(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetIntField, entity, ids->Entity_ticksExisted);
}

extern "C" bool java_get_entity_on_ground(Jvm *jvm, void *entity) {
    return JCALLN(jvm, GetBooleanField, entity, ids->Entity_onGround) != 0;
}

extern "C" bool java_get_entity_is_dead(Jvm *jvm, void *entity) {
    return JCALLN(jvm, GetBooleanField, entity, ids->Entity_isDead) != 0;
}

extern "C" jfloat java_get_entity_fall_distance(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetFloatField, entity, ids->Entity_fallDistance);
}

extern "C" jint java_call_static_entity_list_get_entity_id(Jvm *jvm, void *entity_list) {
    return JCALLN(jvm, CallStaticIntMethod, ids->EntityList, ids->EntityList_getEntityID, entity_list);
}

extern "C" jobject java_get_entity_riding_entity(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetObjectField, entity, ids->Entity_ridingEntity);
}

extern "C" jint java_get_entity_living_base_last_attacker_time(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetIntField, entity_living_base, ids->EntityLivingBase_lastAttackerTime);
}

extern "C" jint java_get_entity_living_base_hurt_time(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetIntField, entity_living_base, ids->EntityLivingBase_hurtTime);
}

extern "C" void java_set_entity_living_base_max_hurt_time(Jvm *jvm, void *entity_living_base, int value) {
    JCALL(jvm, SetIntField, entity_living_base, ids->EntityLivingBase_maxHurtTime, value);
}

extern "C" jfloat java_get_entity_living_base_move_forward(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetFloatField, entity_living_base, ids->EntityLivingBase_moveForward);
}

extern "C" jfloat java_get_entity_living_base_move_strafing(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetFloatField, entity_living_base, ids->EntityLivingBase_moveStrafing);
}

extern "C" jobject java_call_entity_ray_trace(Jvm *jvm, void *object, double value, float a3) {
    return JCALL1(jvm, CallObjectMethod, object, ids->EntityLivingBase_rayTrace, value, (double)a3);
}

extern "C" jfloat java_get_entity_living_base_land_movement_factor(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetFloatField, entity_living_base, ids->EntityLivingBase_landMovementFactor);
}

extern "C" jint get_hurt_time(Jvm *jvm, void *entity_living_base) {
    return JCALL(jvm, GetIntField, entity_living_base, ids->EntityLivingBase_jumpTicks);
}

extern "C" void java_set_entity_living_base_jump_ticks(Jvm *jvm, void *entity_living_base, int value) {
    JCALL(jvm, SetIntField, entity_living_base, ids->EntityLivingBase_jumpTicks, value);
}

extern "C" void java_jni_call_nonvirtual_void_method_entity_living_base_swing_item(Jvm *jvm, void *entity_living_base) {
    JCALLN(jvm, CallNonvirtualVoidMethod, entity_living_base, ids->EntityLivingBase, ids->EntityLivingBase_swingItem);
}

extern "C" jobject java_call_entity_living_base_get_active_potion_effect(Jvm *jvm, void *entity_living_base, void *value) {
    return JCALLN(jvm, CallObjectMethod, entity_living_base, ids->EntityLivingBase_getActivePotionEffect, value);
}

extern "C" void java_entity_player_sp_render_arm_pitch(Jvm *jvm, void *entity_player_sp, Rot value) {
    JNI(jvm)->SetFloatField(jvm->env, entity_player_sp, jvm->ids->EntityPlayerSP_renderArmYaw, value.yaw);
    JNI(jvm)->SetFloatField(jvm->env, entity_player_sp, jvm->ids->EntityPlayerSP_renderArmPitch, value.pitch);
}

extern "C" void java_entity_player_sp_prev_render_arm_pitch(Jvm *jvm, void *entity_player_sp, Rot value) {
    JNI(jvm)->SetFloatField(jvm->env, entity_player_sp, jvm->ids->EntityPlayerSP_prevRenderArmYaw, value.yaw);
    JNI(jvm)->SetFloatField(jvm->env, entity_player_sp, jvm->ids->EntityPlayerSP_prevRenderArmPitch, value.pitch);
}

extern "C" void java_call_entity_player_sp_add_chat_message(Jvm *jvm, void *entity_player_sp, void *value) {
    JCALLN(jvm, CallVoidMethod, entity_player_sp, ids->EntityPlayerSP_addChatMessage, value);
}

extern "C" void java_set_entity_player_sp_time_in_portal(Jvm *jvm, void *entity_player_sp, float value) {
    JCALL(jvm, SetFloatField, entity_player_sp, ids->EntityPlayerSP_timeInPortal, value);
}

extern "C" void java_set_entity_player_sp_prev_time_in_portal(Jvm *jvm, void *entity_player_sp, float value) {
    JCALL(jvm, SetFloatField, entity_player_sp, ids->EntityPlayerSP_prevTimeInPortal, value);
}

extern "C" jobject java_get_entity_player_inventory(Jvm *jvm, void *entity_player) {
    return JCALL(jvm, GetObjectField, entity_player, ids->EntityPlayer_inventory);
}

extern "C" bool java_call_entity_player_is_invisible_to_player(Jvm *jvm, void *entity_player, void *value) {
    return JCALLN(jvm, CallBooleanMethod, entity_player, ids->EntityPlayer_isInvisibleToPlayer, value) != 0;
}

extern "C" jint java_get_entity_player_item_in_use_count(Jvm *jvm, void *entity_player) {
    return JCALL(jvm, GetIntField, entity_player, ids->EntityPlayer_itemInUseCount);
}

extern "C" bool java_call_entity_is_sprinting(Jvm *jvm, void *entity) {
    return JCALLN(jvm, CallBooleanMethod, entity, ids->Entity_isSprinting) != 0;
}

extern "C" bool is_sneaking(Jvm *jvm, void *entity) {
    return JCALLN(jvm, CallBooleanMethod, entity, ids->Entity_isSneaking) != 0;
}

extern "C" jobject java_call_entity_get_formatted_command_sender_name(Jvm *jvm, void *entity) {
    return JCALLN(jvm, CallObjectMethod, entity, ids->Entity_getFormattedCommandSenderName);
}

extern "C" jobject java_call_entity_get_command_sender_name(Jvm *jvm, void *entity) {
    return JCALLN(jvm, CallObjectMethod, entity, ids->Entity_getCommandSenderName);
}

extern "C" jboolean java_call_entity_can_be_collided_with(Jvm *jvm, void *entity) {
    return JCALLN(jvm, CallBooleanMethod, entity, ids->Entity_canBeCollidedWith);
}

extern "C" jobject java_call_entity_living_base_get_team(Jvm *jvm, void *entity_living_base) {
    return JCALLN(jvm, CallObjectMethod, entity_living_base, ids->EntityLivingBase_getTeam);
}

extern "C" bool java_call_entity_player_is_using_item(Jvm *jvm, void *entity_player) {
    return JCALLN(jvm, CallBooleanMethod, entity_player, ids->EntityPlayer_isUsingItem) != 0;
}

extern "C" Vec3 java_get_entity_pos_x(Jvm *jvm, jobject entity) {
    double x = JCALLN(jvm, GetDoubleField, entity, ids->Entity_posX);
    double y = JCALLN(jvm, GetDoubleField, entity, ids->Entity_posY);
    double z = JCALLN(jvm, GetDoubleField, entity, ids->Entity_posZ);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

extern "C" Vec3 java_get_entity_prev_pos_x(Jvm *jvm, jobject entity) {
    double x = JCALLN(jvm, GetDoubleField, entity, ids->Entity_prevPosX);
    double y = JCALLN(jvm, GetDoubleField, entity, ids->Entity_prevPosY);
    double z = JCALLN(jvm, GetDoubleField, entity, ids->Entity_prevPosZ);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

extern "C" Vec3 java_get_entity_motion_x(Jvm *jvm, jobject entity) {
    double x = JCALLN(jvm, GetDoubleField, entity, ids->Entity_motionX);
    double y = JCALLN(jvm, GetDoubleField, entity, ids->Entity_motionY);
    double z = JCALLN(jvm, GetDoubleField, entity, ids->Entity_motionZ);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

extern "C" jobject java_jni_is_instance_of_entity_living_base(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->EntityLivingBase) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_entity_player(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->EntityPlayer) ? candidate : nullptr;
}

// ---- JNI bridge functions found by the gap scan (no caller outside the virtualised init) ----
extern "C" void java_call_entity_set_angles(Jvm *jvm, void *entity, float value, float a3) {
    JCALL1(jvm, CallVoidMethod, entity, ids->Entity_setAngles, ({ double y = value;  y; }), (double)a3);
}

extern "C" jdouble java_get_entity_motion_x_value(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetDoubleField, entity, ids->Entity_motionX);
}

extern "C" void java_call_entity_living_base_jump(Jvm *jvm, void *entity_living_base) {
    JCALLN(jvm, CallVoidMethod, entity_living_base, ids->EntityLivingBase_jump);
}

// ---- gap-scan functions: render-arm rotation of the local player, casts ----
extern "C" Rot java_get_entity_player_sp_render_arm_pitch(Jvm *jvm, jobject entity_player_sp) {   // renderArmYaw / renderArmPitch
    float y = JCALLN(jvm, GetFloatField, entity_player_sp, ids->EntityPlayerSP_renderArmYaw);
    float p = JCALLN(jvm, GetFloatField, entity_player_sp, ids->EntityPlayerSP_renderArmPitch);
    return Rot{y, p};
}
extern "C" Rot java_get_entity_player_sp_prev_render_arm_pitch(Jvm *jvm, jobject entity_player_sp) {   // prevRenderArmYaw / prevRenderArmPitch
    float y = JCALLN(jvm, GetFloatField, entity_player_sp, ids->EntityPlayerSP_prevRenderArmYaw);
    float p = JCALLN(jvm, GetFloatField, entity_player_sp, ids->EntityPlayerSP_prevRenderArmPitch);
    return Rot{y, p};
}
extern "C" jobject java_jni_is_instance_of_entity_player_sp(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->EntityPlayerSP) ? candidate : nullptr;
}

// ---- functions the first gap scan missed (padding / tail-jump neighbours) ----
extern "C" jdouble java_get_entity_motion_y(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetDoubleField, entity, ids->Entity_motionY);
}

extern "C" jdouble java_get_entity_motion_z(Jvm *jvm, void *entity) {
    return JCALL(jvm, GetDoubleField, entity, ids->Entity_motionZ);
}