// Teams - treat players of your team (by name colour) as teammates.
//
// Every tick the module finds your own team colour (from the tab list, else your name tag)
// and tags each player whose name colour matches it ("match_self_color"), or whose colour is
// ticked in the colour list, as a teammate (entity flag 1). The combat modules leave
// teammates alone.
//
// Callbacks (descriptor slots +0x48 / +0x98 / +0x1e8):
//   teams_on_tick      every tick: tag teammates
//   teams_on_toggle    module switched off: untag everyone
//   teams_on_shutdown  module unloaded: free the colour list
#include "slinky/engine.h"
#include "slinky/jids.h"

// ---- settings ----
extern "C" unsigned char teams_match_self_color_value;   // "match_self_color"
extern "C" void *teams_colour_table;                     // the ticked colours (sorted list of 0..15)
extern "C" char key_colour_vector_header[];

// ---- module state ----
extern "C" char teams_module_descriptor[];

struct EventHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- game / engine functions ----
FN(void *, java_call_minecraft_get_net_handler, (void *jvm, void *minecraft));
FN(void *, java_get_net_handler_play_client, (void *jvm, void *net_handler_play_client));    // the tab list (uuid -> info)
FN(void *, java_get_entity_uuid, (void *jvm, void *entity));
FN(void *, java_call_map_get, (void *jvm, void *object, void *key));
FN(void *, java_get_network_player_info_display_name, (void *jvm, void *network_player_info));
FN(void *, java_call_ichat_component_get_formatted_text, (void *jvm, void *object));
FN(unsigned short, name_prefix_code, (void *jvm, void *object));       // first colour code in a text
FN(void *, java_call_network_player_info_get_player_team, (void *jvm, void *network_player_info));
FN(unsigned short, name_last_code, (void *jvm, void *object));         // the team's colour code
FN(unsigned short, entity_name_colour, (void *jvm, void *entity));     // colour code of the name tag
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void, entity_flags_set, (void *jvm, void *entity, int on));
FN(void, entity_flags_clear, (void *jvm, void *entity, int on));
FN(void *, binary_search, (void *table, int n, void *key, void *val, void *cb));
FN(void, compare_signed_chars, ());
FN(void, libc_free, (void *ptr));

enum { FLAG_TEAMMATE = 1, COLOUR_RESET = 'r' };

static bool is_ourselves(Ctx *ctx, void *entity) {
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    return (*env)->IsSameObject(env, ctx->player, entity);
}

// Our own team colour code ('0'..'9', 'a'..'f', or 'r').
static unsigned short own_team_colour(Ctx *ctx) {
    if (!**(int **)ctx->jvm)
        return entity_name_colour(ctx->jvm, ctx->player);
    // Our tab list entry: its display name's colour, else the team's colour, else our name tag's.
    void *net_handler = java_call_minecraft_get_net_handler(ctx->jvm, ctx->minecraft);
    void *tab_list = java_get_net_handler_play_client(ctx->jvm, net_handler);
    void *uuid = java_get_entity_uuid(ctx->jvm, ctx->player);
    void *info = java_call_map_get(ctx->jvm, tab_list, uuid);
    if (!info)
        return entity_name_colour(ctx->jvm, ctx->player);
    void *display_name = java_get_network_player_info_display_name(ctx->jvm, info);
    if (display_name) {
        unsigned short code = name_prefix_code(ctx->jvm, java_call_ichat_component_get_formatted_text(ctx->jvm, display_name));
        return code ? code : COLOUR_RESET;
    }
    void *team = java_call_network_player_info_get_player_team(ctx->jvm, info);
    return team ? name_last_code(ctx->jvm, team) : entity_name_colour(ctx->jvm, ctx->player);
}

extern "C" void teams_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    if (ctx->player == nullptr)
        return;
    unsigned short own_colour = own_team_colour(ctx);

    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (is_ourselves(ctx, other))
            continue;
        unsigned short colour = entity_name_colour(ctx->jvm, other);
        if (teams_match_self_color_value == 1 && colour == own_colour) {
            entity_flags_set(ctx->jvm, other, FLAG_TEAMMATE);
            continue;
        }
        // Colour code character -> number 0..15 ('0'..'9' -> 0..9, 'a'..'f' -> 10..15).
        signed char code = (signed char)colour;
        char index = (char)((code >= 'a' ? 0xa9 : 0xd0) + code);
        if (binary_search(teams_colour_table, 1, key_colour_vector_header, &index, (void *)compare_signed_chars))
            entity_flags_set(ctx->jvm, other, FLAG_TEAMMATE);
        else
            entity_flags_clear(ctx->jvm, other, FLAG_TEAMMATE);
    }
}

// On disable: untag everyone.
extern "C" void teams_on_toggle(EventHook *hook) {
    if (hook->ev != teams_module_descriptor || hook->on)
        return;
    Ctx *ctx = hook->ctx;
    if (!ctx->world)
        return;
    void *players = java_get_world_player_entities(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, players); i++) {
        void *other = java_call_list_get(ctx->jvm, players, i);
        if (!is_ourselves(ctx, other))
            entity_flags_clear(ctx->jvm, other, FLAG_TEAMMATE);
    }
}

extern "C" void teams_on_shutdown(Hook *hook) {
    libc_free(teams_colour_table);
    // Clear the list's begin and end pointers.
    *(__int128 *)&teams_colour_table = 0;
}
