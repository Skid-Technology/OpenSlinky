// Anti Bot - spot fake players (server-side bots) so the combat modules ignore them.
//
// Every tick each player is checked; a player that looks like a bot is tagged (entity flag 2).
// A player counts as a bot when (each check has its own setting):
//   "always_stationary":  it has never moved (less than 1 block in total)
//   "invalid_uuid":       it has a version-2 UUID (1: only while stationary, 2: always)
//   "sleeping":           it is in a bed
//   "always_invis":       it was invisible and without armour since it appeared
//   "min_age":            it has existed for fewer ticks than this
//   "invalid_username":   its name has no letter, digit or "_"
// What we learn about each player is kept in a table by UUID (cleared on a new world).
//
// Callbacks (descriptor slots +0x48 / +0x98):
//   anti_bot_on_tick    every tick: check every player
//   anti_bot_on_toggle  module switched off: clear the table, untag everyone
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Key { unsigned long long hi, lo, padding_10, padding_18; };   // UUID

// What we know about one player.
struct Info {
    unsigned long long hi, lo;   // UUID
    double moved;                // +0x10 total horizontal movement (squared distances summed)
    unsigned char aged;          // +0x18 has passed "min_age"
    unsigned char visible;       // +0x19 has been seen not "always invisible"
    unsigned char name_ok;       // +0x1a its name has a valid character
};

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" float anti_bot_min_age_value;                 // "min_age" (ticks)
extern "C" unsigned char anti_bot_always_stationary_value;   // "always_stationary"
extern "C" int anti_bot_invalid_uuid_value;              // "invalid_uuid": 0 off, 1 while stationary, 2 always
extern "C" unsigned char anti_bot_sleeping_value;        // "sleeping"
extern "C" unsigned char anti_bot_always_invis_value;    // "always_invis"
extern "C" unsigned char anti_bot_invalid_username_value;    // "invalid_username"

// ---- module state ----
extern "C" char anti_bot_module_descriptor[];
extern "C" int anti_bot_world_id_table_belongs;          // which world the table is for
extern "C" void *anti_bot_info_table[1];                 // the table's entries
extern "C" char anti_bot_table_meta[];                   // the table's capacity and count

// ---- game / engine functions ----
FN(int, java_call_object_hash_code, (void *jvm, void *world));
FN(void, libc_free, (void *ptr));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void *, java_get_entity_uuid, (void *jvm, void *entity));
FN(unsigned long long, java_jni_call_long_method_uuid_get_most_significant_bits, (void *jvm, void *uuid));
FN(unsigned long long, java_jni_call_long_method_uuid_get_least_significant_bits, (void *jvm, void *uuid));
FN(Info *, vector_find_insert, (void *table, int keysz, void *meta, void *key, void *hash));   // find, or add a zeroed entry
FN(void, compare_u128, ());
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_player_sleeping, (void *jvm, void *entity_player));
FN(int, java_get_entity_ticks_existed, (void *jvm, void *entity));
FN(unsigned char, java_call_entity_player_is_invisible_to_player, (void *jvm, void *entity_player, void *viewer));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *inv, int slot));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(void *, java_call_entity_get_command_sender_name, (void *jvm, void *entity));
FN(void, entity_flags_set, (void *jvm, void *entity, int tag));
FN(void, entity_flags_clear, (void *jvm, void *entity, int tag));

enum { FLAG_BOT = 2 };
enum { SLOT_BOOTS = 36, SLOT_LEGGINGS = 37, SLOT_CHESTPLATE = 38, SLOT_HELMET = 39 };

static bool is_ourselves(Ctx *ctx, void *entity) {
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    return (*env)->IsSameObject(env, ctx->player, entity);
}

static void clear_table() {
    libc_free(anti_bot_info_table[0]);
    anti_bot_info_table[0] = nullptr;
    __builtin_memset(anti_bot_table_meta, 0, 8);   // capacity + count
}

// Is there an item in this inventory slot?
static bool has_item(void *jvm, void *inventory, int slot) {
    void *stack = java_call_iinventory_get_stack_in_slot(jvm, inventory, slot);
    if (!stack)
        return false;
    void *item = java_call_item_stack_get_item(jvm, stack);
    return item && java_call_static_item_get_id_from_item(jvm, item) != 0;
}

// Does the name have at least one letter, digit or "_"? (Colour codes "§x" are skipped.)
static bool name_has_valid_character(Ctx *ctx, void *player) {
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    void *name = java_call_entity_get_command_sender_name(ctx->jvm, player);
    int length = (*env)->GetStringLength(env, name);
    unsigned short *chars = (unsigned short *)(*env)->GetStringCritical(env, name, nullptr);
    bool valid = false;
    for (int j = 0; j < length; j++) {
        unsigned c = chars[j];
        if (c == 0xa7) {   // "§": skip the colour code character too
            j++;
            continue;
        }
        bool letter = (unsigned short)((c & 0xffdf) - 'A') < 26;   // A-Z or a-z
        if (letter || c == '_' || (unsigned short)(c - '0') <= 9) {
            valid = true;
            break;
        }
    }
    (*env)->ReleaseStringCritical(env, name, chars);
    return valid;
}

// The checks for one player. Updates what we know about it in `info`.
static bool looks_like_bot(Ctx *ctx, void *player, Info *info) {
    // Movement since last tick, added up.
    Vec3 previous = java_get_entity_prev_pos_x(ctx->jvm, player);
    Vec3 current = java_get_entity_pos_x(ctx->jvm, player);
    double dx = previous.x - current.x, dz = previous.z - current.z;
    double moved = dx * dx + dz * dz + info->moved;
    info->moved = moved;
    bool stationary = 1.0 > moved;

    if (anti_bot_always_stationary_value == 1 && stationary)
        return true;
    if (anti_bot_invalid_uuid_value != 0) {
        bool version_2_uuid = ((unsigned)info->hi & 0xf000) == 0x2000;
        if (version_2_uuid && ((anti_bot_invalid_uuid_value == 2) | stationary))
            return true;
    }
    if (anti_bot_sleeping_value == 1 && java_get_entity_player_sleeping(ctx->jvm, player))
        return true;

    int age = java_get_entity_ticks_existed(ctx->jvm, player);
    if (anti_bot_always_invis_value == 1 && !info->visible) {
        // Invisible and no armour at all.
        void *jvm = ctx->jvm;
        if (java_call_entity_player_is_invisible_to_player(jvm, player, ctx->player)) {
            void *inventory = java_get_entity_player_inventory(jvm, player);
            if (!has_item(jvm, inventory, SLOT_BOOTS) && !has_item(jvm, inventory, SLOT_LEGGINGS) &&
                !has_item(jvm, inventory, SLOT_CHESTPLATE) && !has_item(jvm, inventory, SLOT_HELMET))
                return true;
        }
        if (age >= 21)
            info->visible = 1;   // passed the check for a second: stop checking
    }
    float min_age = anti_bot_min_age_value;
    if (min_age > 0.0f && !info->aged) {
        if (min_age > (float)age)
            return true;
        info->aged = 1;
    }

    if (anti_bot_invalid_username_value == 1 && !info->name_ok) {
        if (name_has_valid_character(ctx, player))
            info->name_ok = 1;
        if (!info->name_ok)
            return true;
    }
    return false;
}

extern "C" void anti_bot_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;

    // A different world: start a new table.
    int world_id = ctx->world ? java_call_object_hash_code(ctx->jvm, ctx->world) : 0;
    if (world_id != anti_bot_world_id_table_belongs) {
        clear_table();
        anti_bot_world_id_table_belongs = world_id;
    }
    if (!ctx->world)
        return;

    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (is_ourselves(ctx, other))
            continue;
        void *uuid = java_get_entity_uuid(ctx->jvm, other);
        alignas(16) Key key;
        key.hi = java_jni_call_long_method_uuid_get_most_significant_bits(ctx->jvm, uuid);
        key.lo = java_jni_call_long_method_uuid_get_least_significant_bits(ctx->jvm, uuid);
        key.padding_10 = key.padding_18 = 0;
        Info *info = vector_find_insert(anti_bot_info_table, 0x20, anti_bot_table_meta, &key, (void *)compare_u128);
        if (looks_like_bot(ctx, other, info))
            entity_flags_set(ctx->jvm, other, FLAG_BOT);
        else
            entity_flags_clear(ctx->jvm, other, FLAG_BOT);
    }
}

// On disable: clear the table and untag everyone.
extern "C" void anti_bot_on_toggle(EvHook *hook) {
    if (hook->ev != anti_bot_module_descriptor || hook->on)
        return;
    clear_table();
    Ctx *ctx = hook->ctx;
    if (!ctx->world)
        return;
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (!is_ourselves(ctx, other))
            entity_flags_clear(ctx->jvm, other, FLAG_BOT);
    }
}
