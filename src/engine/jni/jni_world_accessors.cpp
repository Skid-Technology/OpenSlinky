// JNI: world and chunk accessors.
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"
struct Rot { float yaw, pitch; };
struct Vec3 { double x, y, z; };
FN(Vec3, vec3_make, (double x, double y, double z));
extern "C" jobject java_get_network_player_info_display_name(Jvm *jvm, void *network_player_info) {
    return JCALL(jvm, GetObjectField, network_player_info, ids->NetworkPlayerInfo_displayName);
}

extern "C" jobject java_new_chat_component_text_init(Jvm *jvm, void *chat_component_text) {
    return JCALLN(jvm, NewObject, ids->ChatComponentText, ids->ChatComponentText__init_, chat_component_text);
}

extern "C" jobject java_call_iinventory_get_stack_in_slot(Jvm *jvm, void *i_inventory, int value) {
    return JCALLN(jvm, CallObjectMethod, i_inventory, ids->IInventory_getStackInSlot, value);
}

extern "C" jobject java_get_container_player_craft_matrix(Jvm *jvm, void *container_player) {
    return JCALL(jvm, GetObjectField, container_player, ids->ContainerPlayer_craftMatrix);
}

extern "C" jint java_get_slot_slot_number(Jvm *jvm, void *slot) {
    return JCALL(jvm, GetIntField, slot, ids->Slot_slotNumber);
}

extern "C" jobject java_get_inventory_basic(Jvm *jvm, void *inventory_basic) {
    return JCALL(jvm, GetObjectField, inventory_basic, ids->InventoryBasic_Ljava_lang_String_);
}

extern "C" jobject java_call_channel_pipeline_get(Jvm *jvm, void *channel_pipeline, void *value) {
    return JCALLN(jvm, CallObjectMethod, channel_pipeline, ids->ChannelPipeline_get, value);
}

extern "C" jobject java_call_channel_pipeline_remove(Jvm *jvm, void *channel_pipeline, void *value) {
    return JCALLN(jvm, CallObjectMethod, channel_pipeline, ids->ChannelPipeline_remove, value);
}

extern "C" bool java_get_c03_packet_player_moving(Jvm *jvm, void *c03_packet_player) {
    return JCALLN(jvm, GetBooleanField, c03_packet_player, ids->C03PacketPlayer_moving) != 0;
}

extern "C" bool java_get_c03_packet_player_on_ground(Jvm *jvm, void *c03_packet_player) {
    return JCALLN(jvm, GetBooleanField, c03_packet_player, ids->C03PacketPlayer_onGround) != 0;
}

extern "C" jdouble java_get_c03_packet_player_b(Jvm *jvm, void *c03_packet_player) {
    return JCALL(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_b);
}

extern "C" jobject java_get_c08_packet_player_block_placement_stack(Jvm *jvm, void *c08_packet_player_block_placement) {
    return JCALL(jvm, GetObjectField, c08_packet_player_block_placement, ids->C08PacketPlayerBlockPlacement_stack);
}

extern "C" jint java_get_s12_packet_entity_velocity_entity_id(Jvm *jvm, void *s12_packet_entity_velocity) {
    return JCALL(jvm, GetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_entityID);
}

extern "C" jint java_get_s14_packet_entity_entity_id(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetIntField, s14_packet_entity, ids->S14PacketEntity_entityId);
}

extern "C" jbyte java_jni_get_byte_field_s14_packet_entity_pos_x(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetByteField, s14_packet_entity, ids->S14PacketEntity_posX);
}

extern "C" jbyte java_jni_get_byte_field_s14_packet_entity_pos_y(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetByteField, s14_packet_entity, ids->S14PacketEntity_posY);
}

extern "C" jbyte java_jni_get_byte_field_s14_packet_entity_pos_z(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetByteField, s14_packet_entity, ids->S14PacketEntity_posZ);
}

extern "C" jbyte java_jni_get_byte_field_s14_packet_entity_yaw(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetByteField, s14_packet_entity, ids->S14PacketEntity_yaw);
}

extern "C" bool java_get_s14_packet_entity_g(Jvm *jvm, void *s14_packet_entity) {
    return JCALLN(jvm, GetBooleanField, s14_packet_entity, ids->S14PacketEntity_g) != 0;
}

extern "C" jint java_get_s18_packet_entity_teleport_entity_id(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetIntField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_entityId);
}

extern "C" jint java_get_s18_packet_entity_teleport_pos_x(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetIntField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_posX);
}

extern "C" jint java_get_s18_packet_entity_teleport_pos_y(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetIntField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_posY);
}

extern "C" jint java_get_s18_packet_entity_teleport_pos_z(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetIntField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_posZ);
}

extern "C" jbyte java_jni_get_byte_field_s18_packet_entity_teleport_yaw(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetByteField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_yaw);
}

extern "C" jint java_get_s19_packet_entity_status_entity_id(Jvm *jvm, void *s19_packet_entity_status) {
    return JCALL(jvm, GetIntField, s19_packet_entity_status, ids->S19PacketEntityStatus_entityId);
}

extern "C" jfloat java_get_s27_packet_explosion_f(Jvm *jvm, void *s27_packet_explosion) {
    return JCALL(jvm, GetFloatField, s27_packet_explosion, ids->S27PacketExplosion_f);
}

extern "C" jfloat java_get_s27_packet_explosion_h(Jvm *jvm, void *s27_packet_explosion) {
    return JCALL(jvm, GetFloatField, s27_packet_explosion, ids->S27PacketExplosion_h);
}

extern "C" jobject java_get_c16_enum_state_open_inventory(Jvm *jvm) {
    return JCALL(jvm, GetStaticObjectField, ids->C16PacketClientStatus_EnumState, ids->C16PacketClientStatus_EnumState_openInventory);
}

extern "C" jobject java_new_c16_packet_client_status(Jvm *jvm, void *object) {
    return JCALLN(jvm, NewObject, ids->C16PacketClientStatus, ids->C16PacketClientStatus_init, object);
}

extern "C" jobject java_call_network_player_info_get_player_team(Jvm *jvm, void *network_player_info) {
    return JCALLN(jvm, CallObjectMethod, network_player_info, ids->NetworkPlayerInfo_getPlayerTeam);
}

extern "C" jint java_call_object_hash_code(Jvm *jvm, void *object) {
    return JCALLN(jvm, CallIntMethod, object, ids->Object_hashCode);
}

extern "C" jobject java_call_ichat_component_get_formatted_text(Jvm *jvm, void *i_chat_component) {
    return JCALLN(jvm, CallObjectMethod, i_chat_component, ids->IChatComponent_getFormattedText);
}

extern "C" jobject java_call_slot_get_stack(Jvm *jvm, void *slot) {
    return JCALLN(jvm, CallObjectMethod, slot, ids->Slot_getStack);
}

extern "C" jobject java_call_channel_pipeline(Jvm *jvm, void *channel) {
    return JCALLN(jvm, CallObjectMethod, channel, ids->Channel_pipeline);
}

extern "C" jobject java_jni_is_instance_of_inventory_basic(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->InventoryBasic) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c03_packet_player(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C03PacketPlayer) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c07_packet_player_digging(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C07PacketPlayerDigging) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c08_packet_player_block_placement(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C08PacketPlayerBlockPlacement) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c02_packet_use_entity(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C02PacketUseEntity) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c0_bpacket_entity_action(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C0BPacketEntityAction) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s12_packet_entity_velocity(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S12PacketEntityVelocity) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s14_packet_entity(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S14PacketEntity) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s18_packet_entity_teleport(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S18PacketEntityTeleport) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s19_packet_entity_status(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S19PacketEntityStatus) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s29_packet_sound_effect(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S29PacketSoundEffect) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s06_packet_update_health(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S06PacketUpdateHealth) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s08_packet_player_pos_look(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S08PacketPlayerPosLook) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s27_packet_explosion(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S27PacketExplosion) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s00_packet_keep_alive(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S00PacketKeepAlive) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c00_packet_keep_alive(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C00PacketKeepAlive) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c0_epacket_click_window(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C0EPacketClickWindow) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c16_packet_client_status(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C16PacketClientStatus) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->c) ? candidate : nullptr;
}

extern "C" Vec3 java_get_c03_packet_player_a(Jvm *jvm, jobject c03_packet_player) {
    double x = JCALLN(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_a);
    double y = JCALLN(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_b);
    double z = JCALLN(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_c);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

// ---- hand-written
FN(jint, java_call_enum_ordinal, (Jvm *jvm, void *c07_packet_player_digging));     // enum ordinal

// enum field: object (ordinal via java_call_enum_ordinal) in the newer mappings, plain int in the old ones
extern "C" jint java_c07_packet_player_digging_status(Jvm *jvm, jobject object) {
    JIds *ids = jvm->ids; JNIEnv *env = jvm->env;
    auto vt = *env; jfieldID id = (jfieldID)ids->C07PacketPlayerDigging_status;
    if (ids->variant != 0)
        return java_call_enum_ordinal(jvm, vt->GetObjectField(env, object, id));
    return vt->GetIntField(env, object, id);
}
extern "C" jint java_c0_bpacket_entity_action(Jvm *jvm, jobject object) {
    JIds *ids = jvm->ids; JNIEnv *env = jvm->env;
    auto vt = *env; jfieldID id = (jfieldID)ids->C0BPacketEntityAction_Lis_a_;
    if (ids->variant != 0)
        return java_call_enum_ordinal(jvm, vt->GetObjectField(env, object, id));
    return vt->GetIntField(env, object, id);
}
extern "C" jobject java_call_channel_pipeline_add_before(Jvm *jvm, jobject channel_pipeline, const char *a, const char *b, void *c) {
    JNIEnv *env = jvm->env;
    jstring sa = (*env)->NewStringUTF(env, a);
    jstring sb = (*env)->NewStringUTF(env, b);
    return JCALL1(jvm, CallObjectMethod, channel_pipeline, (jmethodID)ids->ChannelPipeline_addBefore, sa, sb, c);
}
extern "C" jint java_get_c16_packet_client_status_status(Jvm *jvm, jobject c16_packet_client_status) {
    jobject e = JCALLN(jvm, GetObjectField, c16_packet_client_status, (jfieldID)ids->C16PacketClientStatus_status);
    return java_call_enum_ordinal(jvm, e);
}

// ---- JNI bridge functions found by the gap scan (no caller outside the virtualised init) ----
extern "C" jobject java_call_static_ichat_component_serializer_json_to_component(Jvm *jvm, void *i_chat_component) {
    return JCALLN(jvm, CallStaticObjectMethod, ids->IChatComponent_Serializer, ids->IChatComponent_Serializer_jsonToComponent, i_chat_component);
}

extern "C" jdouble java_get_c03_packet_player_x(Jvm *jvm, void *c03_packet_player) {
    return JCALL(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_a);
}

extern "C" jdouble java_get_c03_packet_player_c(Jvm *jvm, void *c03_packet_player) {
    return JCALL(jvm, GetDoubleField, c03_packet_player, ids->C03PacketPlayer_c);
}

extern "C" jint java_get_s12_packet_entity_velocity_motion_x(Jvm *jvm, void *s12_packet_entity_velocity) {
    return JCALL(jvm, GetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionX);
}

extern "C" jbyte java_jni_get_byte_field_s14_packet_entity_pitch(Jvm *jvm, void *s14_packet_entity) {
    return JCALL(jvm, GetByteField, s14_packet_entity, ids->S14PacketEntity_pitch);
}

extern "C" jbyte java_jni_get_byte_field_s18_packet_entity_teleport_pitch(Jvm *jvm, void *s18_packet_entity_teleport) {
    return JCALL(jvm, GetByteField, s18_packet_entity_teleport, ids->S18PacketEntityTeleport_pitch);
}

extern "C" void java_set_s27_packet_explosion_f(Jvm *jvm, void *s27_packet_explosion, float value) {
    JCALL(jvm, SetFloatField, s27_packet_explosion, ids->S27PacketExplosion_f, value);
}

extern "C" void java_set_s27_packet_explosion_h(Jvm *jvm, void *s27_packet_explosion, float value) {
    JCALL(jvm, SetFloatField, s27_packet_explosion, ids->S27PacketExplosion_h, value);
}

extern "C" jobject java_jni_is_instance_of_s3_fpacket_custom_payload(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S3FPacketCustomPayload) ? candidate : nullptr;
}

// ---- functions the first gap scan missed (padding / tail-jump neighbours) ----
extern "C" void java_set_s12_packet_entity_velocity_motion_x(Jvm *jvm, void *s12_packet_entity_velocity, int value) {
    JCALL(jvm, SetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionX, value);
}

extern "C" jint java_get_s12_packet_entity_velocity_motion_y(Jvm *jvm, void *s12_packet_entity_velocity) {
    return JCALL(jvm, GetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionY);
}

extern "C" void java_set_s12_packet_entity_velocity_motion_y(Jvm *jvm, void *s12_packet_entity_velocity, int value) {
    JCALL(jvm, SetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionY, value);
}

extern "C" jint java_get_s12_packet_entity_velocity_motion_z(Jvm *jvm, void *s12_packet_entity_velocity) {
    return JCALL(jvm, GetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionZ);
}

extern "C" void java_set_s12_packet_entity_velocity_motion_z(Jvm *jvm, void *s12_packet_entity_velocity, int value) {
    JCALL(jvm, SetIntField, s12_packet_entity_velocity, ids->S12PacketEntityVelocity_motionZ, value);
}

extern "C" jfloat java_get_s27_packet_explosion_g(Jvm *jvm, void *s27_packet_explosion) {
    return JCALL(jvm, GetFloatField, s27_packet_explosion, ids->S27PacketExplosion_g);
}

extern "C" void java_set_s27_packet_explosion_g(Jvm *jvm, void *s27_packet_explosion, float value) {
    JCALL(jvm, SetFloatField, s27_packet_explosion, ids->S27PacketExplosion_g, value);
}

extern "C" jobject java_get_s3_fpacket_custom_payload(Jvm *jvm, void *s3_f_packet_custom_payload) {
    return JCALL(jvm, GetObjectField, s3_f_packet_custom_payload, ids->S3FPacketCustomPayload_Ljava_lang_String_);
}

extern "C" jint java_call_packet_buffer_readable_bytes(Jvm *jvm, void *packet_buffer) {
    return JCALLN(jvm, CallIntMethod, packet_buffer, ids->PacketBuffer_readableBytes);
}

extern "C" jint java_call_packet_buffer_reader_index(Jvm *jvm, void *packet_buffer) {
    return JCALLN(jvm, CallIntMethod, packet_buffer, ids->PacketBuffer_readerIndex);
}

// S3FPacketCustomPayload payload as a byte[]: 1.7 keeps a byte[] field; 1.8 keeps a PacketBuffer whose readable
// bytes are copied into a new array (getBytes(readerIndex, array))
extern "C" jobject java_get_s3_fpacket_custom_payload_data(Jvm *jvm, void *pkt) {
    jobject data = JCALLN(jvm, GetObjectField, pkt, ids->S3FPacketCustomPayload_data);
    if (jvm->ids->variant == 0) return data;
    jint len = JCALLN(jvm, CallIntMethod, data, ids->PacketBuffer_readableBytes);
    jint idx = JCALLN(jvm, CallIntMethod, data, ids->PacketBuffer_readerIndex);
    jobject arr = JCALLN(jvm, NewByteArray, len);
    if (!arr) return nullptr;
    JCALLN(jvm, CallObjectMethod, data, ids->PacketBuffer_getBytes, idx, arr);
    return arr;
}

extern "C" jobject java_call_packet_buffer_get_bytes(Jvm *jvm, void *buf, int idx, jobject arr) {
    return JCALL1(jvm, CallObjectMethod, buf, ids->PacketBuffer_getBytes, idx, arr);
}