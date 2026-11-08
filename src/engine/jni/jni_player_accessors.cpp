// JNI: local-player accessors.
// cflags: /Ob1 /clang:-fno-slp-vectorize
#include "slinky/engine.h"
#include "slinky/jcall.h"
struct Rot { float yaw, pitch; };
struct Vec3 { double x, y, z; };
FN(Vec3, vec3_make, (double x, double y, double z));

extern "C" jint java_get_game_settings_third_person_view(Jvm *jvm, void *game_settings) {
    return JCALL(jvm, GetIntField, game_settings, ids->GameSettings_thirdPersonView);
}

extern "C" void java_set_game_settings_third_person_view(Jvm *jvm, void *game_settings, int value) {
    JCALL(jvm, SetIntField, game_settings, ids->GameSettings_thirdPersonView, value);
}

extern "C" jint java_get_key_binding_press_time(Jvm *jvm, void *key_binding) {
    return JCALL(jvm, GetIntField, key_binding, ids->KeyBinding_pressTime);
}

extern "C" void java_set_key_binding_press_time(Jvm *jvm, void *key_binding, int value) {
    JCALL(jvm, SetIntField, key_binding, ids->KeyBinding_pressTime, value);
}

extern "C" jint get_event_key(Jvm *jvm, void *key_binding) {
    return JCALL(jvm, GetIntField, key_binding, ids->KeyBinding_keyCode);
}

extern "C" bool is_key_down(Jvm *jvm, void *key_binding) {
    return JCALLN(jvm, GetBooleanField, key_binding, ids->KeyBinding_pressed) != 0;
}

extern "C" void java_set_key_binding_pressed(Jvm *jvm, void *key_binding, unsigned char value) {
    JCALL(jvm, SetBooleanField, key_binding, ids->KeyBinding_pressed, value);
}

extern "C" bool java_call_static_keyboard_is_key_down(Jvm *jvm, int keyboard) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Keyboard, ids->Keyboard_isKeyDown, keyboard) != 0;
}

extern "C" bool java_call_static_mouse_is_button_down(Jvm *jvm, int mouse) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Mouse, ids->Mouse_isButtonDown, mouse) != 0;
}

extern "C" jfloat get_partial_ticks(Jvm *jvm, void *timer) {
    return JCALL(jvm, GetFloatField, timer, ids->Timer_renderPartialTicks);
}

extern "C" jfloat java_get_timer_timer_speed(Jvm *jvm, void *timer) {
    return JCALL(jvm, GetFloatField, timer, ids->Timer_timerSpeed);
}

extern "C" void java_set_timer_timer_speed(Jvm *jvm, void *timer, float value) {
    JCALL(jvm, SetFloatField, timer, ids->Timer_timerSpeed, value);
}

extern "C" jfloat java_get_timer_elapsed_partial_ticks(Jvm *jvm, void *timer) {
    return JCALL(jvm, GetFloatField, timer, ids->Timer_elapsedPartialTicks);
}

extern "C" void java_set_timer_elapsed_partial_ticks(Jvm *jvm, void *timer, float value) {
    JCALL(jvm, SetFloatField, timer, ids->Timer_elapsedPartialTicks, value);
}

extern "C" jint java_get_inventory_player_current_item(Jvm *jvm, void *inventory_player) {
    return JCALL(jvm, GetIntField, inventory_player, ids->InventoryPlayer_currentItem);
}

extern "C" void java_set_inventory_player_current_item(Jvm *jvm, void *inventory_player, int value) {
    JCALL(jvm, SetIntField, inventory_player, ids->InventoryPlayer_currentItem, value);
}

extern "C" jobject java_get_inventory_player_main_inventory(Jvm *jvm, void *inventory_player) {
    return JCALL(jvm, GetObjectField, inventory_player, ids->InventoryPlayer_mainInventory);
}

extern "C" jobject java_get_inventory_player_item_stack(Jvm *jvm, void *inventory_player) {
    return JCALL(jvm, GetObjectField, inventory_player, ids->InventoryPlayer_itemStack);
}

extern "C" jobject java_get_gui_container_the_slot(Jvm *jvm, void *gui_container) {
    return JCALL(jvm, GetObjectField, gui_container, ids->GuiContainer_theSlot);
}

extern "C" bool java_jni_is_instance_of_gui_inventory(Jvm *jvm, void *candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, ids->GuiInventory) != 0;
}

extern "C" bool java_jni_is_instance_of_gui_chest(Jvm *jvm, void *candidate) {
    return JCALLN(jvm, IsInstanceOf, candidate, ids->GuiChest) != 0;
}

extern "C" jobject java_get_gui_chest_lower_chest_inventory(Jvm *jvm, void *gui_chest) {
    return JCALL(jvm, GetObjectField, gui_chest, ids->GuiChest_lowerChestInventory);
}

extern "C" jint java_call_static_item_get_id_from_item(Jvm *jvm, void *item) {
    return JCALLN(jvm, CallStaticIntMethod, ids->Item, ids->Item_getIdFromItem, item);
}

extern "C" jfloat java_call_item_stack_get_str_vs_block(Jvm *jvm, void *item_stack, void *value) {
    return JCALLN(jvm, CallFloatMethod, item_stack, ids->ItemStack_getStrVsBlock, value);
}

extern "C" jint java_get_item_stack_stack_size(Jvm *jvm, void *item_stack) {
    return JCALL(jvm, GetIntField, item_stack, ids->ItemStack_stackSize);
}

extern "C" jint java_get_item_stack_metadata(Jvm *jvm, void *item_stack) {
    return JCALL(jvm, GetIntField, item_stack, ids->ItemStack_metadata);
}

extern "C" jobject java_get_net_handler_play_client(Jvm *jvm, void *net_handler_play_client) {
    return JCALL(jvm, GetObjectField, net_handler_play_client, ids->NetHandlerPlayClient_playerInfoMap);
}

extern "C" jobject java_get_net_handler_play_client_net_manager(Jvm *jvm, void *net_handler_play_client) {
    return JCALL(jvm, GetObjectField, net_handler_play_client, ids->NetHandlerPlayClient_netManager);
}

extern "C" jobject java_get_network_manager(Jvm *jvm, void *network_manager) {
    return JCALL(jvm, GetObjectField, network_manager, ids->NetworkManager_Lio_netty_channel_Channel_);
}

extern "C" void send_packet(Jvm *jvm, void *a1, void *value) {
    JCALLN(jvm, CallVoidMethod, a1, ids->NetworkManager_sendPacket, value);
}

extern "C" bool keyboard_has_next(Jvm *jvm) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Keyboard, ids->Keyboard_next) != 0;
}

extern "C" bool keyboard_can_repeat(Jvm *jvm) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Keyboard, ids->Keyboard_getEventKeyState) != 0;
}

extern "C" jint keyboard_next_key(Jvm *jvm) {
    return JCALLN(jvm, CallStaticIntMethod, ids->Keyboard, ids->Keyboard_getEventKey);
}

extern "C" bool java_call_static_mouse_is_grabbed(Jvm *jvm) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Mouse, ids->Mouse_isGrabbed) != 0;
}

extern "C" bool keyboard_has_events(Jvm *jvm) {
    return JCALLN(jvm, CallStaticBooleanMethod, ids->Mouse, ids->Mouse_next) != 0;
}

extern "C" jobject java_call_item_stack_get_item(Jvm *jvm, void *item_stack) {
    return JCALLN(jvm, CallObjectMethod, item_stack, ids->ItemStack_getItem);
}

extern "C" jobject java_call_item_stack_get_enchantment_tag_list(Jvm *jvm, void *item_stack) {
    return JCALLN(jvm, CallObjectMethod, item_stack, ids->ItemStack_getEnchantmentTagList);
}

extern "C" jint java_call_item_stack_get_max_stack_size(Jvm *jvm, void *item_stack) {
    return JCALLN(jvm, CallIntMethod, item_stack, ids->ItemStack_getMaxStackSize);
}

extern "C" bool java_call_item_stack_has_display_name(Jvm *jvm, void *item_stack) {
    return JCALLN(jvm, CallBooleanMethod, item_stack, ids->ItemStack_hasDisplayName) != 0;
}

extern "C" Vec3 java_get_vec3_x_coord(Jvm *jvm, jobject vec3) {
    double x = JCALLN(jvm, GetDoubleField, vec3, ids->Vec3_xCoord);
    double y = JCALLN(jvm, GetDoubleField, vec3, ids->Vec3_yCoord);
    double z = JCALLN(jvm, GetDoubleField, vec3, ids->Vec3_zCoord);
    Vec3 r = vec3_make(x, y, z);
    asm volatile("");
    return r;
}

extern "C" jobject java_get_game_settings_sneak_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 7);
}

extern "C" jobject get_held_item(Jvm *jvm, jobject object) {
    jobject a = JCALLN(jvm, GetObjectField, object, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)a, 6);
}

extern "C" jobject java_get_game_settings_forward_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 2);
}

extern "C" jobject java_get_game_settings_back_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 4);
}

extern "C" jobject java_get_game_settings_left_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 3);
}

extern "C" jobject java_get_game_settings_right_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 5);
}

extern "C" jobject java_get_game_settings_attack_key(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, 0);
}

// ---- hand-written
struct Box { Vec3 lo, hi; };
FN(Box, aabb_make, (const Vec3 *lo, const Vec3 *hi));
FN(bool, java_call_static_keyboard_is_key_down, (Jvm *j, int key));     // Keyboard.isKeyDown
FN(bool, java_call_static_mouse_is_button_down, (Jvm *j, int button));  // Mouse.isButtonDown

extern "C" jobject java_get_game_settings_key_17_or_8(Jvm *jvm, jobject game_settings) {
    jobject object = JCALLN(jvm, GetObjectField, game_settings, (jfieldID)ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)object, (ids->variant == 0) * 9 + 8);
}
extern "C" jobject get_game_screen(Jvm *jvm, jobject object) {
    jobject a = JCALLN(jvm, GetObjectField, object, (jfieldID)ids->GameSettings_keyBindings);
    return JCALL(jvm, GetObjectArrayElement, (jobjectArray)a, ids->variant == 0 ? 9 : 10);
}
extern "C" void java_increment_key_binding_press_time(Jvm *jvm, jobject key_binding) {
    jint v = JCALLN(jvm, GetIntField, key_binding, (jfieldID)ids->KeyBinding_pressTime);
    JCALL(jvm, SetIntField, key_binding, (jfieldID)ids->KeyBinding_pressTime, v + 1);
}
// key binding pressed: negative codes are mouse buttons (code + 100)
extern "C" bool java_get_key_binding_key_code(Jvm *jvm, jobject key_binding) {
    jint keyboard = JCALLN(jvm, GetIntField, key_binding, (jfieldID)ids->KeyBinding_keyCode);
    if (keyboard < 0) return java_call_static_mouse_is_button_down(jvm, keyboard + 100);
    if ((unsigned)keyboard <= 0x100) return java_call_static_keyboard_is_key_down(jvm, keyboard);
    return false;
}
extern "C" jobject java_new_vec3_init(Jvm *jvm, Vec3 v) {
    return JCALL2(jvm, NewObject, (jclass)ids->Vec3, (jmethodID)ids->Vec3__init_, v.x, v.y, v.z);
}
extern "C" jobject java_new_axis_aligned_bb_init(Jvm *jvm, Box b) {
    return JCALL2(jvm, NewObject, (jclass)ids->AxisAlignedBB, (jmethodID)ids->AxisAlignedBB__init_,
                             b.lo.x, b.lo.y, b.lo.z, b.hi.x, b.hi.y, b.hi.z);
}
extern "C" Box java_get_axis_aligned_bb_min_x(Jvm *jvm, jobject axis_aligned_bb) {
    double x = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_minX);
    double y = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_minY);
    double z = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_minZ);
    Vec3 lo = vec3_make(x, y, z);
    x = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_maxX);
    y = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_maxY);
    z = JCALLN(jvm, GetDoubleField, axis_aligned_bb, (jfieldID)ids->AxisAlignedBB_maxZ);
    Vec3 hi = vec3_make(x, y, z);
    Box r = aabb_make(&lo, &hi);

    return r;
}
extern "C" void java_call_static_mouse_set_grabbed(Jvm *jvm, bool grab) {
    JCALL2(jvm, CallStaticVoidMethod, (jclass)ids->Mouse, (jmethodID)ids->Mouse_setGrabbed, grab);
}
extern "C" void java_call_static_mouse_set_cursor_position(Jvm *jvm, int x, int y) {
    JCALL2(jvm, CallStaticVoidMethod, (jclass)ids->Mouse, (jmethodID)ids->Mouse_setCursorPosition, x, y);
}
extern "C" void keyboard_send_key(Jvm *jvm, jobject object, unsigned short c, int k) {
    JCALL1(jvm, CallVoidMethod, object, (jmethodID)ids->GuiScreen_keyTyped, (int)c, k);
}
extern "C" void java_gui_container_handle_mouse_click(Jvm *jvm, jobject object, int a, int b, int c) {
    JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->GuiContainer_handleMouseClick, 0, a, b, c);
}
extern "C" bool java_call_static_item_stack_are_item_stack_tags_equal(Jvm *jvm, void *item_stack, void *other_object) {
    return JCALL2(jvm, CallStaticBooleanMethod, (jclass)ids->ItemStack, (jmethodID)ids->ItemStack_areItemStackTagsEqual, item_stack, other_object) != 0;
}
extern "C" void send_packet_with_id(Jvm *jvm, jobject object, void *other_object, void *b) {
    JNI(jvm)->CallVoidMethod(jvm->env, object, (jmethodID)jvm->ids->NetworkManager_scheduleOutboundPacket, other_object, b);
}
// newer mappings take an extra (empty) array argument
extern "C" void java_call_generic_future_listener(Jvm *jvm, jobject generic_future_listener, void *other_object) {
    if (jvm->ids->NetworkManager_scheduleOutboundPacket) {
        jobjectArray arr = JNI(jvm)->NewObjectArray(jvm->env, 0, (jclass)jvm->ids->GenericFutureListener, nullptr);
        JCALL1(jvm, CallVoidMethod, generic_future_listener, (jmethodID)ids->NetworkManager_scheduleOutboundPacket, other_object, arr);
    } else {
        JCALL1(jvm, CallVoidMethod, generic_future_listener, (jmethodID)ids->NetworkManager_sendPacket, other_object);
    }
}

// ---- JNI bridge functions found by the gap scan (no caller outside the virtualised init) ----
extern "C" jint java_get_timer_elapsed_ticks(Jvm *jvm, void *timer) {
    return JCALL(jvm, GetIntField, timer, ids->Timer_elapsedTicks);
}

extern "C" void java_jni_set_long_field_gui_container_last_click_time(Jvm *jvm, void *gui_container, long long value) {
    JCALL(jvm, SetLongField, gui_container, ids->GuiContainer_lastClickTime, (jlong)value);
}

extern "C" jobject java_call_static_item_get_item_by_id(Jvm *jvm, int item) {
    return JCALLN(jvm, CallStaticObjectMethod, ids->Item, ids->Item_getItemById, item);
}

extern "C" void java_set_item_stack_stack_size(Jvm *jvm, void *item_stack, int value) {
    JCALL(jvm, SetIntField, item_stack, ids->ItemStack_stackSize, value);
}

// ---- functions the first gap scan missed (padding / tail-jump neighbours) ----
extern "C" jobject java_new_item_stack_init(Jvm *jvm, void *item_stack) {
    return JCALLN(jvm, NewObject, ids->ItemStack, ids->ItemStack__init_, item_stack);
}