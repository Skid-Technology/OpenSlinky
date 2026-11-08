// Friends - your friend list. Other modules leave friends alone.
//
// Friends are stored by player UUID in a sorted table. Every tick each player in the world
// is tagged (entity flag 4) when it is a friend; the combat modules check that tag.
// With "middle_click_add", middle-clicking a player adds or removes them.
//
// Callbacks:
//   friends_on_middle_click_add  middle click: add / remove the player under the crosshair
//   friends_on_tick              every tick: tag friends in the world (and update "last seen")
//   friends_on_toggle            module switched off: untag everyone
//   friends_on_shutdown          module unloaded: free the table
#include "slinky/engine.h"
#include "slinky/jids.h"

// A table key: the UUID (two 64-bit halves) and when we last saw them.
struct Key { unsigned long long hi, lo, seen, padding_18; };   // 0x20 bytes
struct Entry { char padding_00[0x10]; long long seen; };        // a table entry (same layout)

// A notification for the GUI ("friend added" / "friend removed").
struct Notice { int kind; int padding_04; unsigned char added; char rest[0x3f]; };   // 0x48 bytes
enum { NOTICE_FRIEND = 0x13 };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" unsigned char friends_middle_click_add_value;   // "middle_click_add"

// ---- module state ----
extern "C" char friends_module_descriptor[];
extern "C" void *friend_set_friend_table;    // the sorted table (begin, end pointers)
extern "C" char friend_set_table_meta[];

// ---- game / engine functions ----
FN(void *, get_minecraft, (void *jvm));
FN(void *, java_get_minecraft_object_mouse_over, (void *jvm, void *minecraft));
FN(void *, java_get_moving_object_position_entity_hit, (void *jvm, void *moving_object_position));
FN(void *, java_jni_is_instance_of_entity_other_player_mp, (void *jvm, void *candidate));   // the entity as a player, or null
FN(void *, java_get_entity_uuid, (void *jvm, void *entity));
FN(unsigned long long, java_jni_call_long_method_uuid_get_most_significant_bits, (void *jvm, void *uuid));
FN(unsigned long long, java_jni_call_long_method_uuid_get_least_significant_bits, (void *jvm, void *uuid));
FN(long long, libc_time64, (void *t));                                                       // current time (seconds)
FN(Entry *, binary_search, (void *table, int keysz, void *meta, void *key, void *hash));     // find an entry, or null
FN(void, sorted_insert, (void *table, int keysz, void *meta, void *key, void *hash));
FN(void, vector_erase_value, (void *table, int keysz, void *meta, void *entry));
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mode));                    // flag 2: bot
FN(void, dispatch_event, (void *ui, Notice *n));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void, entity_flags_set, (void *jvm, void *entity, int tag));
FN(void, entity_flags_clear, (void *jvm, void *entity, int tag));
FN(void, compare_friend_key, ());                                                            // compares the UUID part of two keys
FN(void, libc_free, (void *ptr));

enum { FLAG_BOT = 2, FLAG_FRIEND = 4 };

static bool is_ourselves(Ctx *ctx, void *entity) {
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    return (*env)->IsSameObject(env, ctx->player, entity);
}

// Middle click: add or remove the player under the crosshair.
extern "C" void friends_on_middle_click_add(void *jvm, void *mgr) {
    if (friends_middle_click_add_value != 1)
        return;
    void *hit = java_get_minecraft_object_mouse_over(jvm, get_minecraft(jvm));
    if (!hit)
        return;
    void *entity = java_get_moving_object_position_entity_hit(jvm, hit);
    if (!entity)
        return;
    void *player = java_jni_is_instance_of_entity_other_player_mp(jvm, entity);
    if (!player)
        return;

    void *uuid = java_get_entity_uuid(jvm, player);
    Key key;
    key.hi = java_jni_call_long_method_uuid_get_most_significant_bits(jvm, uuid);
    key.lo = java_jni_call_long_method_uuid_get_least_significant_bits(jvm, uuid);
    key.seen = libc_time64(nullptr);
    key.padding_18 = 0;

    Notice notice = {};
    Entry *entry = binary_search(friend_set_friend_table, 0x20, friend_set_table_meta, &key, (void *)compare_friend_key);
    if (entry) {
        // Already a friend: remove.
        notice.kind = NOTICE_FRIEND;
        notice.added = 0;
        vector_erase_value(friend_set_friend_table, 0x20, friend_set_table_meta, entry);
    } else {
        // Bots can't be friends.
        if (entity_flags_has(jvm, player, FLAG_BOT))
            return;
        notice.kind = NOTICE_FRIEND;
        notice.added = 1;
        sorted_insert(&friend_set_friend_table, 0x20, friend_set_table_meta, &key, (void *)compare_friend_key);
    }
    dispatch_event(mgr, &notice);
}

// Every tick: tag the friends in the world.
extern "C" void friends_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (!ctx->player)
        return;
    long long now = libc_time64(nullptr);
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (is_ourselves(ctx, other))
            continue;
        void *uuid = java_get_entity_uuid(ctx->jvm, other);
        alignas(16) Key key;
        key.hi = java_jni_call_long_method_uuid_get_most_significant_bits(ctx->jvm, uuid);
        key.lo = java_jni_call_long_method_uuid_get_least_significant_bits(ctx->jvm, uuid);
        key.seen = 0;
        key.padding_18 = 0;
        Entry *entry = binary_search(friend_set_friend_table, 0x20, friend_set_table_meta, &key, (void *)compare_friend_key);
        if (entry) {
            entity_flags_set(ctx->jvm, other, FLAG_FRIEND);
            entry->seen = now;
        } else {
            entity_flags_clear(ctx->jvm, other, FLAG_FRIEND);
        }
    }
}

// On disable: untag everyone.
extern "C" void friends_on_toggle(EvHook *hook) {
    if (hook->ev != friends_module_descriptor || hook->on)
        return;
    Ctx *ctx = hook->ctx;
    if (!ctx->world)
        return;
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (!is_ourselves(ctx, other))
            entity_flags_clear(ctx->jvm, other, FLAG_FRIEND);
    }
}

extern "C" void friends_on_shutdown(Hook *hook) {
    libc_free(friend_set_friend_table);
    // Clear the table's begin and end pointers.
    *(__int128 *)&friend_set_friend_table = 0;
}
