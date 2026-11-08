// JNI: render-manager accessors (camera fields).
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"
struct Rot { float yaw, pitch; };
struct Vec3 { double x, y, z; };
extern "C" jobject java_new_c_init(Jvm *jvm, int a1) {
    return JCALLN(jvm, NewObject, ids->c, ids->c__init_, a1);
}

extern "C" jobject java_get_c01_packet_chat_message(Jvm *jvm, void *c01_packet_chat_message) {
    return JCALL(jvm, GetObjectField, c01_packet_chat_message, ids->C01PacketChatMessage_Ljava_lang_String_);
}

extern "C" void java_set_c01_packet_chat_message(Jvm *jvm, void *c01_packet_chat_message, void *value) {
    JCALL(jvm, SetObjectField, c01_packet_chat_message, ids->C01PacketChatMessage_Ljava_lang_String_, value);
}

extern "C" jobject java_get_c14_packet_tab_complete(Jvm *jvm, void *c14_packet_tab_complete) {
    return JCALL(jvm, GetObjectField, c14_packet_tab_complete, ids->C14PacketTabComplete_Ljava_lang_String_);
}

extern "C" jobject java_get_render_manager_instance(Jvm *jvm) {
    return JCALL(jvm, GetStaticObjectField, ids->RenderManager, ids->RenderManager_instance);
}

extern "C" jfloat java_get_render_manager_player_view_x(Jvm *jvm, void *render_manager) {
    return JCALL(jvm, GetFloatField, render_manager, ids->RenderManager_playerViewX);
}

extern "C" void java_set_render_manager_player_view_x(Jvm *jvm, void *render_manager, float value) {
    JCALL(jvm, SetFloatField, render_manager, ids->RenderManager_playerViewX, value);
}

extern "C" jfloat java_get_render_manager_player_view_y(Jvm *jvm, void *render_manager) {
    return JCALL(jvm, GetFloatField, render_manager, ids->RenderManager_playerViewY);
}

extern "C" void java_set_render_manager_player_view_y(Jvm *jvm, void *render_manager, float value) {
    JCALL(jvm, SetFloatField, render_manager, ids->RenderManager_playerViewY, value);
}

extern "C" jobject java_get_render_manager_text_renderer(Jvm *jvm, void *render_manager) {
    return JCALL(jvm, GetObjectField, render_manager, ids->RenderManager_textRenderer);
}

extern "C" jobject java_get_score_player_team(Jvm *jvm, void *score_player_team) {
    return JCALL(jvm, GetObjectField, score_player_team, ids->ScorePlayerTeam_Ljava_lang_String_);
}

extern "C" void call_get_mouse_over(Jvm *jvm, void *entity_renderer, float value) {
    JCALL1(jvm, CallVoidMethod, entity_renderer, ids->EntityRenderer_getMouseOver, (double)value);
}

extern "C" void call_orient_camera(Jvm *jvm, void *entity_renderer, float value) {
    JCALL1(jvm, CallVoidMethod, entity_renderer, ids->EntityRenderer_orientCamera, (double)value);
}

extern "C" void set_render_manager_boolean(Jvm *jvm, void *render_global, unsigned char value) {
    JCALL(jvm, SetBooleanField, render_global, ids->RenderGlobal_displayListEntitiesDirty, value);
}

extern "C" jboolean java_call_renderer_living_entity_can_render_name(Jvm *jvm, void *renderer_living_entity, void *value) {
    return JCALLN(jvm, CallBooleanMethod, renderer_living_entity, ids->RendererLivingEntity_canRenderName, value);
}

extern "C" jint java_call_font_renderer_get_string_width(Jvm *jvm, void *font_renderer, void *value) {
    return JCALLN(jvm, CallIntMethod, font_renderer, ids->FontRenderer_getStringWidth, value);
}

extern "C" void java_set_potion_effect_duration(Jvm *jvm, void *potion_effect, int value) {
    JCALL(jvm, SetIntField, potion_effect, ids->PotionEffect_duration, value);
}

extern "C" jint java_get_static_display_width(Jvm *jvm) {
    return JCALL(jvm, GetStaticIntField, ids->Display, ids->Display_width);
}

extern "C" jint java_get_static_display_height(Jvm *jvm) {
    return JCALL(jvm, GetStaticIntField, ids->Display, ids->Display_height);
}

extern "C" jint display_width(Jvm *jvm) {
    return JCALL(jvm, GetStaticIntField, ids->Display, ids->Display_framebufferWidth);
}

extern "C" jint display_height(Jvm *jvm) {
    return JCALL(jvm, GetStaticIntField, ids->Display, ids->Display_framebufferHeight);
}

extern "C" jfloat java_get_render_item_z_level(Jvm *jvm, void *render_item) {
    return JCALL(jvm, GetFloatField, render_item, ids->RenderItem_zLevel);
}

extern "C" void java_set_render_item_z_level(Jvm *jvm, void *render_item, float value) {
    JCALL(jvm, SetFloatField, render_item, ids->RenderItem_zLevel, value);
}

extern "C" jfloat java_get_player_controller_mp_cur_block_damage_mp(Jvm *jvm, void *player_controller_mp) {
    return JCALL(jvm, GetFloatField, player_controller_mp, ids->PlayerControllerMP_curBlockDamageMP);
}

extern "C" void java_set_player_controller_mp_cur_block_damage_mp(Jvm *jvm, void *player_controller_mp, float value) {
    JCALL(jvm, SetFloatField, player_controller_mp, ids->PlayerControllerMP_curBlockDamageMP, value);
}

extern "C" jint java_get_player_controller_mp_block_hit_delay(Jvm *jvm, void *player_controller_mp) {
    return JCALL(jvm, GetIntField, player_controller_mp, ids->PlayerControllerMP_blockHitDelay);
}

extern "C" void java_set_player_controller_mp_block_hit_delay(Jvm *jvm, void *player_controller_mp, int value) {
    JCALL(jvm, SetIntField, player_controller_mp, ids->PlayerControllerMP_blockHitDelay, value);
}

extern "C" jboolean java_get_player_controller_mp_is_hitting_block(Jvm *jvm, void *player_controller_mp) {
    return JCALL(jvm, GetBooleanField, player_controller_mp, ids->PlayerControllerMP_isHittingBlock);
}

extern "C" void java_set_player_controller_mp_is_hitting_block(Jvm *jvm, void *player_controller_mp, unsigned char value) {
    JCALL(jvm, SetBooleanField, player_controller_mp, ids->PlayerControllerMP_isHittingBlock, value);
}

extern "C" jobject java_call_nbttag_list_get_compound_tag_at(Jvm *jvm, void *nbt_tag_list, int value) {
    return JCALLN(jvm, CallObjectMethod, nbt_tag_list, ids->NBTTagList_getCompoundTagAt, value);
}

extern "C" jshort java_jni_call_short_method_nbttag_compound_get_short(Jvm *jvm, void *nbt_tag_compound, void *value) {
    return JCALLN(jvm, CallShortMethod, nbt_tag_compound, ids->NBTTagCompound_getShort, value);
}

extern "C" jboolean java_get_entity_arrow_in_ground(Jvm *jvm, void *entity_arrow) {
    return JCALL(jvm, GetBooleanField, entity_arrow, ids->EntityArrow_inGround);
}

extern "C" jobject java_call_static_stat_collector_translate_to_local(Jvm *jvm, void *stat_collector) {
    return JCALLN(jvm, CallStaticObjectMethod, ids->StatCollector, ids->StatCollector_translateToLocal, stat_collector);
}

extern "C" jint java_call_enum_ordinal(Jvm *jvm, void *enum_object) {
    return JCALLN(jvm, CallIntMethod, enum_object, ids->Enum_ordinal);
}

extern "C" void gl_begin_2d(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableCull);
}

extern "C" void gl_disable_lighting(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableLighting);
}

extern "C" void gl_enable_depth(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableDepth);
}

extern "C" void gl_push_state(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableDepth);
}

extern "C" void gl_pop_state(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableBlend);
}

extern "C" void gl_bind_texture(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableTexture2D);
}

extern "C" void gl_clear_color(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableTexture2D);
}

extern "C" void gl_depth_mask(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableAlpha);
}

extern "C" void java_call_static_gl_state_manager_disable_alpha(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableAlpha);
}

extern "C" void tick_netty(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->Display, ids->Display_update);
}

extern "C" bool world_is_loaded(Jvm *jvm) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Display, ids->Display_isActive) != 0;
}

extern "C" jobject java_new_render_item(Jvm *jvm) {
    return JCALLN(jvm, NewObject, ids->RenderItem, ids->RenderItem_init);
}

extern "C" void java_call_static_render_helper_enable_standard_item_lighting(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->RenderHelper, ids->RenderHelper_enableStandardItemLighting);
}

extern "C" void java_call_static_render_helper_disable_standard_item_lighting(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->RenderHelper, ids->RenderHelper_disableStandardItemLighting);
}

extern "C" jobject java_call_entity_item_get_entity_item(Jvm *jvm, void *entity_item) {
    return JCALLN(jvm, CallObjectMethod, entity_item, ids->EntityItem_getEntityItem);
}

extern "C" jlong java_jni_call_long_method_uuid_get_least_significant_bits(Jvm *jvm, void *uuid) {
    return JCALLN(jvm, CallLongMethod, uuid, ids->UUID_getLeastSignificantBits);
}

extern "C" jlong java_jni_call_long_method_uuid_get_most_significant_bits(Jvm *jvm, void *uuid) {
    return JCALLN(jvm, CallLongMethod, uuid, ids->UUID_getMostSignificantBits);
}

extern "C" void gui_key_passthrough(Jvm *jvm, void *player_controller_mp) {
    JCALLN(jvm, CallVoidMethod, player_controller_mp, ids->PlayerControllerMP_updateController);
}

extern "C" jint java_call_nbttag_list_tag_count(Jvm *jvm, void *nbt_tag_list) {
    return JCALLN(jvm, CallIntMethod, nbt_tag_list, ids->NBTTagList_tagCount);
}

extern "C" jobject java_jni_is_instance_of_c01_packet_chat_message(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C01PacketChatMessage) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_c14_packet_tab_complete(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->C14PacketTabComplete) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_s2_apacket_particles(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->S2APacketParticles) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_score_player_team(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->ScorePlayerTeam) ? candidate : nullptr;
}

extern "C" jobject java_jni_is_instance_of_entity_item(Jvm *jvm, jobject candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, (jclass)ids->EntityItem) ? candidate : nullptr;
}

// ---- hand-written
FN(float, libc_roundf, (float v));

extern "C" float java_call_entity_renderer_get_fovmodifier(Jvm *jvm, jobject entity_renderer, float partial) {
    return JCALL1(jvm, CallFloatMethod, entity_renderer, (jmethodID)ids->EntityRenderer_getFOVModifier, (double)partial, 1);
}
extern "C" void set_active_render_info(Jvm *jvm, void *active_render_info, bool b) {
    JCALL2(jvm, CallStaticVoidMethod, (jclass)ids->ActiveRenderInfo, (jmethodID)ids->ActiveRenderInfo_updateRenderInfo, active_render_info, (int)b);
}
// MS x64 passes a {double,double} struct by reference, so declaring this as one D2 while every caller
// (chams_render_living_entity, hook_render_player) passes three separate doubles made the callee read the
// stale R9 as a pointer (fault at 0). Keep the three doubles in the signature: they are the method's
// three D arguments and go to the variadic JNI call as such.
extern "C" void java_jni_call_nonvirtual_renderer_living_entity_do_render(Jvm *jvm, jobject object, void *other_object, double x, double d0, double d1, float f0, float f1) {
    JCALL2(jvm, CallNonvirtualVoidMethod, object, (jclass)ids->RendererLivingEntity, (jmethodID)ids->RendererLivingEntity___2, other_object, x, d0, d1,
           (double)f0, (double)f1);
}
// FontRenderer.drawString: float coordinates in the newer mappings, rounded ints in the old ones
extern "C" jint java_font_draw_string(Jvm *jvm, jobject font, jobject str, float x, float y, unsigned color, unsigned char shadow) {
    if (jvm->ids->variant != 0)
        return JNI(jvm)->CallIntMethod(jvm->env, font, (jmethodID)jvm->ids->FontRenderer_drawString, str, (double)x, (double)y, color, (int)shadow);
    int ix = (int)libc_roundf(x);
    int iy = (int)libc_roundf(y);
    return JNI(jvm)->CallIntMethod(jvm->env, font, (jmethodID)jvm->ids->FontRenderer_drawString, str, ix, iy, color, (int)shadow);
}
extern "C" void gl_blend_mode(Jvm *jvm, int a, int b) {
    JCALL2(jvm, CallStaticVoidMethod, (jclass)ids->GlStateManager, (jmethodID)ids->GlStateManager_blendFunc, a, b);
}
extern "C" void gl_set_color(Jvm *jvm, float a, float b, float c, float d) {
    JCALL2(jvm, CallStaticVoidMethod, (jclass)ids->GlStateManager, (jmethodID)ids->GlStateManager_color, (double)a, (double)b, (double)c, (double)d);
}
// element k (< 256) of a static object array
extern "C" jobject java_potion_potion_types(Jvm *jvm, unsigned k) {
    if (k > 0xff) return nullptr;
    JIds *ids = jvm->ids;
    JNIEnv *env = jvm->env;
    jobject object = (*env)->GetStaticObjectField(env, (jclass)ids->Potion, (jfieldID)ids->Potion_potionTypes);
    if (!object) return nullptr;
    jobject value = (*env)->GetObjectArrayElement(env, (jobjectArray)object, k);
    (*env)->DeleteLocalRef(env, object);
    return value;
}
extern "C" void java_render_item_init(Jvm *jvm, jobject object, void *other_object, void *b, void *c, int d, int e) {
    if (jvm->ids->variant != 0)
        JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->RenderItem__init_, c, d, e);
    else
        JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->RenderItem__init_, other_object, b, c, d, e);
}
extern "C" void java_render_item_render_item_overlay_into_gui(Jvm *jvm, jobject object, void *other_object, void *b, void *c, int d, int e, void *f) {
    if (jvm->ids->variant != 0)
        JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->RenderItem_renderItemOverlayIntoGUI, other_object, c, d, e, f);
    else
        JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->RenderItem_renderItemOverlayIntoGUI, other_object, b, c, d, e, f);
}

// ---- JNI bridge functions found by the gap scan (no caller outside the virtualised init) ----
extern "C" void java_call_framebuffer_bind_framebuffer(Jvm *jvm, void *framebuffer, unsigned char value) {
    JCALL1(jvm, CallVoidMethod, framebuffer, ids->Framebuffer_bindFramebuffer, value);
}

extern "C" void java_call_static_gl_state_manager_disable_cull(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableCull);
}

extern "C" void java_call_static_gl_state_manager_enable_lighting(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_enableLighting);
}

extern "C" void java_call_static_gl_state_manager_disable_blend(Jvm *jvm) {
    JCALLN(jvm, CallStaticVoidMethod, ids->GlStateManager, ids->GlStateManager_disableBlend);
}

extern "C" void java_call_entity_item_search_for_other_items_nearby(Jvm *jvm, void *entity_item) {
    JCALLN(jvm, CallVoidMethod, entity_item, ids->EntityItem_searchForOtherItemsNearby);
}

// EntityRenderer.renderHand(partialTicks, pass)
extern "C" void java_call_entity_renderer_render_hand(Jvm *jvm, jobject renderer, float partial_ticks, int pass) {
    JCALL1(jvm, CallVoidMethod, renderer, ids->EntityRenderer_renderHand, (double)partial_ticks, pass);
}