// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
extern "C" void *friend_set_friend_table; extern "C" char friend_set_table_meta[]; extern "C" int friend_set_dirty_count; extern "C" unsigned char friends_friends_setting_field_0x7c;
FN(void, vector_erase, (void *vec, int elem, void *hdr, void *key));
FN(void *, java_jni_is_instance_of_entity_player, (void *jvm, void *candidate));
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mode));
FN(void *, java_call_renderer_living_entity_can_render_name, (void *jvm, void *renderer_living_entity, void *other_object));
extern "C" void friend_set_remove_by_key(void *, void *key) { vector_erase(friend_set_friend_table, 0x20, friend_set_table_meta, key); }
extern "C" void friend_set_clear_dirty() { friend_set_dirty_count = 0; }
extern "C" void *should_render_player_name(void *jvm, void *render_player, void *other_object) {
    if (friends_friends_setting_field_0x7c == 1 && java_jni_is_instance_of_entity_player(jvm, other_object) != 0 && !entity_flags_has(jvm, other_object, 2)) return 0;
    return java_call_renderer_living_entity_can_render_name(jvm, render_player, other_object);
}
