// JNI: entity field accessors.
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"
struct Rot { float yaw, pitch; };
struct Vec3 { double x, y, z; };
FN(Vec3, vec3_make, (double x, double y, double z));
extern "C" void java_set_entity_player_item_in_use_count(Jvm *jvm, void *entity_player, int value) {
    JCALL(jvm, SetIntField, entity_player, ids->EntityPlayer_itemInUseCount, value);
}

extern "C" jobject java_call_entity_player_get_equipment_in_slot(Jvm *jvm, void *entity_player, int value) {
    return JCALLN(jvm, CallObjectMethod, entity_player, ids->EntityPlayer_getEquipmentInSlot, value);
}

extern "C" jobject java_get_entity_player_inventory_container(Jvm *jvm, void *entity_player) {
    return JCALL(jvm, GetObjectField, entity_player, ids->EntityPlayer_inventoryContainer);
}

extern "C" bool java_get_entity_player_sleeping(Jvm *jvm, void *entity_player) {
    return JCALLN(jvm, GetBooleanField, entity_player, ids->EntityPlayer_sleeping) != 0;
}

extern "C" jfloat java_get_entity_player_speed_on_ground(Jvm *jvm, void *entity_player) {
    return JCALL(jvm, GetFloatField, entity_player, ids->EntityPlayer_speedOnGround);
}

extern "C" void java_set_entity_player_speed_on_ground(Jvm *jvm, void *entity_player, float value) {
    JCALL(jvm, SetFloatField, entity_player, ids->EntityPlayer_speedOnGround, value);
}

extern "C" jint java_get_entity_other_player_mp_other_player_mppos_rotation_increments(Jvm *jvm, void *entity_other_player_mp) {
    return JCALL(jvm, GetIntField, entity_other_player_mp, ids->EntityOtherPlayerMP_otherPlayerMPPosRotationIncrements);
}

extern "C" jdouble java_get_entity_other_player_mp_other_player_mpyaw(Jvm *jvm, void *entity_other_player_mp) {
    return JCALL(jvm, GetDoubleField, entity_other_player_mp, ids->EntityOtherPlayerMP_otherPlayerMPYaw);
}

extern "C" jobject get_block_state_id(Jvm *jvm, void *block) {
    return JCALLN(jvm, CallStaticObjectMethod, ids->Block, ids->Block_getBlockFromItem, block);
}

extern "C" jint get_block_id(Jvm *jvm, void *block) {
    return JCALLN(jvm, CallStaticIntMethod, ids->Block, ids->Block_getIdFromBlock, block);
}

extern "C" jobject java_get_tile_entity_block_type(Jvm *jvm, void *tile_entity) {
    return JCALL(jvm, GetObjectField, tile_entity, ids->TileEntity_blockType);
}

extern "C" jobject java_get_world_player_entities(Jvm *jvm, void *world) {
    return JCALL(jvm, GetObjectField, world, ids->World_playerEntities);
}

extern "C" jobject java_get_world_loaded_entity_list(Jvm *jvm, void *world) {
    return JCALL(jvm, GetObjectField, world, ids->World_loadedEntityList);
}

extern "C" jobject java_get_world_loaded_tile_entity_list(Jvm *jvm, void *world) {
    return JCALL(jvm, GetObjectField, world, ids->World_loadedTileEntityList);
}

extern "C" jobject java_get_chunk_storage_arrays(Jvm *jvm, void *chunk) {
    return JCALL(jvm, GetObjectField, chunk, ids->Chunk_storageArrays);
}

extern "C" jint java_get_chunk_x_position(Jvm *jvm, void *chunk) {
    return JCALL(jvm, GetIntField, chunk, ids->Chunk_xPosition);
}

extern "C" jint java_get_chunk_z_position(Jvm *jvm, void *chunk) {
    return JCALL(jvm, GetIntField, chunk, ids->Chunk_zPosition);
}

extern "C" jobject java_get_id_b20(Jvm *jvm, void *object) {
    return JCALL(jvm, GetObjectField, object, ids->ExtendedBlockStorage_blockLSBArray);
}

extern "C" jobject java_get_chunk_provider_client(Jvm *jvm, void *chunk_provider_client) {
    return JCALL(jvm, GetObjectField, chunk_provider_client, ids->ChunkProviderClient_chunkMapping);
}

extern "C" jobject java_get_moving_object_position_hit_vec(Jvm *jvm, void *moving_object_position) {
    return JCALL(jvm, GetObjectField, moving_object_position, ids->MovingObjectPosition_hitVec);
}

extern "C" jobject java_get_moving_object_position_entity_hit(Jvm *jvm, void *moving_object_position) {
    return JCALL(jvm, GetObjectField, moving_object_position, ids->MovingObjectPosition_entityHit);
}

extern "C" jobject java_call_list_get(Jvm *jvm, void *list, int value) {
    return JCALLN(jvm, CallObjectMethod, list, ids->List_get, value);
}

extern "C" jobject java_call_map_get(Jvm *jvm, void *map, void *value) {
    return JCALLN(jvm, CallObjectMethod, map, ids->Map_get, value);
}

extern "C" jfloat java_get_game_settings_mouse_sensitivity(Jvm *jvm, void *game_settings) {
    return JCALL(jvm, GetFloatField, game_settings, ids->GameSettings_mouseSensitivity);
}

extern "C" jobject get_block_from_pos(Jvm *jvm, void *a1) {
    return JCALLN(jvm, CallObjectMethod, a1, ids->_init__getBlock);
}

extern "C" void update_entities(Jvm *jvm, void *world) {
    JCALLN(jvm, CallVoidMethod, world, ids->World_updateEntities);
}

extern "C" jint java_call_list_size(Jvm *jvm, void *list) {
    return JCALLN(jvm, CallIntMethod, list, ids->List_size);
}

extern "C" jobject java_jni_is_instance_of_entity_other_player_mp(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->EntityOtherPlayerMP) ? candidate : nullptr;
}

extern "C" Vec3 java_get_entity_other_player_mp_other_player_mpx(Jvm *jvm, jobject entity_other_player_mp) {
    double x = JCALLN(jvm, GetDoubleField, entity_other_player_mp, ids->EntityOtherPlayerMP_otherPlayerMPX);
    double y = JCALLN(jvm, GetDoubleField, entity_other_player_mp, ids->EntityOtherPlayerMP_otherPlayerMPY);
    double z = JCALLN(jvm, GetDoubleField, entity_other_player_mp, ids->EntityOtherPlayerMP_otherPlayerMPZ);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

// ---- hand-written (BlockPos / Vec3i glue, multi-argument calls)
struct I3 { int x, y, z; };
FN(jobject, get_block_from_pos, (Jvm *j, void *block_pos));

extern "C" __attribute__((noinline)) jobject java_block_pos_init(Jvm *jvm, int x, int y, int z) {
    return JNI(jvm)->NewObject(jvm->env, (jclass)jvm->ids->BlockPos, (jmethodID)jvm->ids->BlockPos__init_, x, y, z);
}
extern "C" Vec3 java_get_block_pos_a(Jvm *jvm, jobject block_pos) {
    double x = JCALLN(jvm, GetIntField, block_pos, (jfieldID)ids->BlockPos_a);
    double y = JCALLN(jvm, GetIntField, block_pos, (jfieldID)ids->BlockPos_c);
    double z = JCALLN(jvm, GetIntField, block_pos, (jfieldID)ids->BlockPos_d);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}
extern "C" __attribute__((noinline)) void java_set_block_pos_a(Jvm *jvm, jobject block_pos, int value, int y, int z) {
    JCALLN(jvm, SetIntField, block_pos, (jfieldID)ids->BlockPos_a, value);
    JCALLN(jvm, SetIntField, block_pos, (jfieldID)ids->BlockPos_c, y);
    JCALL(jvm, SetIntField, block_pos, (jfieldID)ids->BlockPos_d, z);
}
extern "C" I3 get_block_position(Jvm *jvm, jobject object) {
    I3 r;
    r.x = JCALLN(jvm, GetIntField, object, (jfieldID)ids->BlockPos_a);
    r.y = JCALLN(jvm, GetIntField, object, (jfieldID)ids->BlockPos_c);
    r.z = JCALLN(jvm, GetIntField, object, (jfieldID)ids->BlockPos_d);
    return r;
}
// block position of an entity/object; the mapping set decides whether it goes through a BlockPos
extern "C" Vec3 java_get_block_pos_as_vec3(Jvm *jvm, jobject block_pos) {
    int x;
    long yo, zo;
    if (jvm->ids->variant != 0) {
        block_pos = JCALLN(jvm, GetObjectField, block_pos, (jfieldID)ids->TileEntity_pos);
        x = JCALLN(jvm, GetIntField, block_pos, (jfieldID)ids->BlockPos_a);
        zo = 0x468; yo = 0x460;
    } else {
        x = JCALLN(jvm, GetIntField, block_pos, (jfieldID)ids->TileEntity_xCoord);
        zo = 0x780; yo = 0x778;
    }
    double dx = x;
    double dy = JCALLN(jvm, GetIntField, block_pos, *(jfieldID *)((char *)ids + yo));
    double dz = JCALLN(jvm, GetIntField, block_pos, *(jfieldID *)((char *)ids + zo));
    Vec3 r = vec3_make(dx, dy, dz);
    asm volatile("");
    return r;
}
extern "C" jobject java_call_variant(Jvm *jvm, jobject object, int x, int y, int z) {
    if (jvm->ids->variant != 0) {
        jobject p = java_block_pos_init(jvm, x, y, z);
        jobject r = JCALLN(jvm, CallObjectMethod, object, (jmethodID)ids->World_getBlockState, p);
        return get_block_from_pos(jvm, r);
    }
    return JNI(jvm)->CallObjectMethod(jvm->env, object, (jmethodID)jvm->ids->World_getBlock, x, y, z);
}
extern "C" jobject java_call_world_get_block_state(Jvm *jvm, jobject object, jobject pos, int x, int y, int z) {
    if (pos) {
        java_set_block_pos_a(jvm, pos, x, y, z);
        jobject r = JCALLN(jvm, CallObjectMethod, object, (jmethodID)ids->World_getBlockState, pos);
        return get_block_from_pos(jvm, r);
    }
    return JNI(jvm)->CallObjectMethod(jvm->env, object, (jmethodID)jvm->ids->World_getBlock, x, y, z);
}
extern "C" jboolean set_block_state(Jvm *jvm, jobject object, int a, int b, int c, void *d, int e, int f) {
    return JNI(jvm)->CallBooleanMethod(jvm->env, object, (jmethodID)jvm->ids->World_setBlock, a, b, c, d, e, f);
}
extern "C" jboolean set_block_at_pos(Jvm *jvm, jobject object, void *other_object, void *b, int c) {
    return JNI(jvm)->CallBooleanMethod(jvm->env, object, (jmethodID)jvm->ids->World_setBlockState, other_object, b, c);
}
extern "C" jobject java_world_ray_trace_blocks(Jvm *jvm, jobject object, void *other_object, void *b, bool c, bool d, bool e) {
    int c_ = c, d_ = d, e_ = e;

    return JNI(jvm)->CallObjectMethod(jvm->env, object, (jmethodID)jvm->ids->World_rayTraceBlocks, other_object, b, c_, d_, e_);
}
extern "C" jobject java_world_get_chunk_from_chunk_coords(Jvm *jvm, jobject object, int a, int b) {
    return JNI(jvm)->CallObjectMethod(jvm->env, object, (jmethodID)jvm->ids->World_getChunkFromChunkCoords, a, b);
}
extern "C" void update_chunk(Jvm *jvm, jobject object, int a, int b, int c, int d, int e, int f) {
    JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->World_markBlockRangeForRenderUpdate, a, b, c, d, e, f);
}
extern "C" jobject java_get_world_chunk_provider_client(Jvm *jvm, jobject chunk_provider_client) {
    jobject x = JCALLN(jvm, GetObjectField, chunk_provider_client, (jfieldID)ids->World_chunkProvider);
    return JCALLN(jvm, IsInstanceOf, x, (jclass)ids->ChunkProviderClient) ? x : nullptr;
}
extern "C" void set_block_meta(Jvm *jvm, jobject object, int a, int b) {
    JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->ChunkProviderClient_unloadChunk, a, b);
}
extern "C" jobject java_moving_object_position_init(Jvm *jvm, void *moving_object_position, void *other_object) {
    return JNI(jvm)->NewObject(jvm->env, (jclass)jvm->ids->MovingObjectPosition, (jmethodID)jvm->ids->MovingObjectPosition__init_, moving_object_position, other_object);
}

FN(jint, java_call_enum_ordinal, (Jvm *jvm, void *axis_aligned_bb));     // enum ordinal
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));        // a + b
FN(Vec3, java_get_block_pos_a, (Jvm *j, jobject o));
FN(I3, get_block_position, (Jvm *j, jobject block_pos));
extern "C" const Vec3 jni_entity_accessors_facing_unit_offset[];    // facing -> unit offset

extern "C" jobject java_axis_aligned_bb_calculate_intercept(Jvm *jvm, jobject object, void *other_object, void *b) {
    return JNI(jvm)->CallObjectMethod(jvm->env, object, (jmethodID)jvm->ids->AxisAlignedBB_calculateIntercept, other_object, b);
}
extern "C" jint java_get_moving_object_position_type_ordinal(Jvm *jvm, jobject moving_object_position) {
    jobject x = JCALLN(jvm, GetObjectField, moving_object_position, (jfieldID)ids->MovingObjectPosition_typeOfHit);
    return java_call_enum_ordinal(jvm, x);
}
// side hit of a MovingObjectPosition (EnumFacing ordinal)
extern "C" jint java_get_variant(Jvm *jvm, jobject object) {
    if (jvm->ids->variant != 0) {
        jobject x = JCALLN(jvm, GetObjectField, object, (jfieldID)ids->MovingObjectPosition_sideHit);
        return java_call_enum_ordinal(jvm, x);
    }
    return JCALL(jvm, GetIntField, object, (jfieldID)ids->MovingObjectPosition_sideHitIndex);
}
// block position of a MovingObjectPosition
extern "C" Vec3 java_get_moving_object_position_block_x(Jvm *jvm, jobject moving_object_position) {
    if (jvm->ids->variant != 0) {
        jobject block_pos = JCALLN(jvm, GetObjectField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockPos);
        return java_get_block_pos_a(jvm, block_pos);
    }
    double x = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockX);
    double y = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockY);
    double z = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockZ);
    return vec3_make(x, y, z);
}
extern "C" I3 java_get_moving_object_position_block_pos(Jvm *jvm, jobject moving_object_position) {
    if (jvm->ids->variant != 0) {
        jobject p = JCALLN(jvm, GetObjectField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockPos);
        return get_block_position(jvm, p);
    }
    I3 r;
    r.x = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockX);
    r.y = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockY);
    r.z = JCALLN(jvm, GetIntField, moving_object_position, (jfieldID)ids->MovingObjectPosition_blockZ);
    return r;
}
// block position + facing offset (the block the hit side points to)
extern "C" Vec3 java_get_adjacent_block_position(Jvm *jvm, jobject object) {
    Vec3 p = java_get_moving_object_position_block_x(jvm, object);
    unsigned side;
    if (jvm->ids->variant != 0) {
        jobject x = JCALLN(jvm, GetObjectField, object, (jfieldID)ids->MovingObjectPosition_sideHit);
        side = java_call_enum_ordinal(jvm, x);
    } else {
        side = JCALLN(jvm, GetIntField, object, (jfieldID)ids->MovingObjectPosition_sideHitIndex);
    }
    return vec3_add(p, jni_entity_accessors_facing_unit_offset[side]);
}
extern "C" jobject java_get_game_settings_use_item_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, (jfieldID)ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 1);
}
extern "C" jobject java_get_game_settings_key_8_or_9(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, (jfieldID)ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, ids->variant == 0 ? 8 : 9);
}

// ---- JNI bridge functions found by the gap scan (no caller outside the virtualised init) ----
extern "C" jobject java_call_world_get_entity_by_id(Jvm *jvm, void *world, int value) {
    return JCALLN(jvm, CallObjectMethod, world, ids->World_getEntityByID, value);
}

extern "C" jboolean java_call_list_add(Jvm *jvm, void *list, void *value) {
    return JCALLN(jvm, CallBooleanMethod, list, ids->List_add, value);
}

// ---- functions the first gap scan missed (padding / tail-jump neighbours) ----
extern "C" jboolean java_call_list_remove(Jvm *jvm, void *list, void *value) {
    return JCALLN(jvm, CallBooleanMethod, list, ids->List_remove, value);
}

extern "C" bool java_call_map_contains_key(Jvm *jvm, void *map, void *value) {
    return JCALLN(jvm, CallBooleanMethod, map, ids->Map_containsKey, value) != 0;
}