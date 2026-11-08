// Inv Walk - keep walking while an inventory or container screen is open.
//
// Minecraft releases all movement keys when a screen opens. Every tick this module
// presses them again to match the keys the player is really holding.
//
// In "silent" mode the server is never told that the inventory is open while you are
// moving: the "open inventory" packet is held back, and inventory clicks are queued
// until the player stands still. Moving while clicking is what anticheats look for.
//
// Callbacks:
//   inv_walk_on_toggle           module switched on/off: on disable, give the keys back
//   inv_walk_on_gui_screen       every outgoing packet (silent mode): hold back / queue packets
//   inv_walk_on_inventory_tick   every tick: drain keyboard input while queued clicks are replayed
//   inv_walk_tick_keep_movement  every tick: press the movement keys while a screen is open
#include "slinky/engine.h"
#include "slinky/jids.h"

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on, 0 = switched off
};

struct GuiHook {
    char padding_00[8];
    void *a;                 // +0x08 JVM handle
    void *screen_obj;        // +0x10 network manager the packet is sent through
    void *screen;            // +0x18 the packet being sent
    unsigned char handled;   // +0x20 set to 1 to cancel sending it
};

// ---- settings ----
enum InvWalkMode { MODE_VANILLA = 0, MODE_SILENT = 1 };
extern "C" unsigned inv_walk_mode;                   // "mode"
enum StopOnClick { STOP_ALWAYS = 0, STOP_SPRINT_ONLY = 1, STOP_NEVER = 2 };
extern "C" unsigned inv_walk_stop_on_click;          // "stop_on_click": what to do while clicks are pending
extern "C" float inv_walk_stop_duration_value;       // "stop_duration": ticks to keep stopping after a click
extern "C" unsigned inv_walk_stop_non_inv_value;     // "stop_non_inv": same as stop_on_click, for chests and other screens

// ---- module state ----
extern "C" char inv_walk_module_descriptor[];
extern "C" void *inv_walk_queued_clicks[0x20];        // click packets waiting to be sent (JNI global refs)
extern "C" unsigned long long inv_walk_queued_count;  // how many are queued
extern "C" int inv_walk_tick_counter;                 // ticks since start
extern "C" int inv_walk_tick_last_screen_activity;    // tick of the last inventory click

// What the server has been told about the player's inventory (silent mode).
enum InvWalkPhase {
    PHASE_NO_SCREEN = 0,      // no screen open
    PHASE_OTHER_SCREEN = 1,   // a non-inventory screen (chest, furnace, ...) is open
    PHASE_SERVER_CLOSED = 2,  // inventory open, but the server thinks it is closed
    PHASE_SERVER_OPEN = 3,    // inventory open, and the server knows
};
extern "C" int inv_walk_phase;

extern "C" unsigned char inv_walk_replaying_queued_clicks;   // 1 while queued clicks are being sent
extern "C" unsigned char inv_walk_keys_released_last_tick;   // 1 if we forced the movement keys up last tick

// ---- game / engine functions ----
FN(void *, get_current_screen, (void *jvm, void *object));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, java_get_game_settings_forward_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_back_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_left_key, (void *jvm, void *game_settings));
FN(void *, java_get_game_settings_right_key, (void *jvm, void *game_settings));
FN(void *, get_held_item, (void *jvm, void *item));                                    // here: the jump key
FN(void *, java_get_game_settings_key_17_or_8, (void *jvm, void *game_settings));      // the sprint key
FN(unsigned char, java_get_key_binding_key_code, (void *jvm, void *key_binding));     // key physically held down
FN(void, java_set_key_binding_pressed, (void *jvm, void *key_binding, unsigned char on));
FN(unsigned char, java_jni_is_instance_of_gui_inventory, (void *jvm, void *screen));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_minecraft_get_net_handler, (void *jvm, void *minecraft));
FN(void *, java_get_net_handler_play_client_net_manager, (void *jvm, void *net_handler_play_client));
FN(void *, java_get_inventory_player_item_stack, (void *jvm, void *inv));             // item held on the mouse cursor
FN(void *, java_get_entity_player_inventory_container, (void *jvm, void *entity_player));
FN(void *, java_get_container_player_craft_matrix, (void *jvm, void *container_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *inv, int slot));
FN(void *, java_new_c_init, (void *jvm, int x));                                       // new C0D "close window" packet
FN(void, java_call_entity_set_sprinting, (void *jvm, void *entity, bool on));
FN(unsigned char, release_global_refs, (void *jvm, void *object));                     // send the queued clicks; 1 if any were sent
FN(void *, java_jni_is_instance_of_c16_packet_client_status, (void *jvm, void *screen));
FN(int, java_get_c16_packet_client_status_status, (void *jvm, void *c16_packet_client_status));
FN(void *, java_jni_is_instance_of_c0_epacket_click_window, (void *jvm, void *screen));
FN(void *, java_get_c16_enum_state_open_inventory, (void *jvm));
FN(void *, java_new_c16_packet_client_status, (void *jvm, void *object));             // new C16 "open inventory" packet
FN(void, java_call_generic_future_listener, (void *jvm, void *generic_future_listener, void *other_object));   // send a packet
FN(void *, java_jni_is_instance_of_c, (void *jvm, void *screen));                     // is it a C0D "close window" packet
FN(unsigned char, keyboard_has_events, (void *jvm));
FN(unsigned char, keyboard_has_next, (void *jvm));

// On disable: give the movement keys back to the player.
extern "C" void inv_walk_on_toggle(EvHook *hook) {
    if (hook->on || hook->ev != inv_walk_module_descriptor)
        return;
    Ctx *ctx = hook->ctx;
    release_global_refs(ctx->jvm, ctx->minecraft);
}

// Keep a reference to the packet so it can be sent later (at most 32 are kept).
static void queue_packet(void *jvm, void *packet) {
    if (inv_walk_queued_count == 0x20)
        return;
    JNIEnv *env = ((Jvm *)jvm)->env;
    void *ref = (*env)->NewGlobalRef(env, packet);
    if (ref) {
        unsigned long long index = inv_walk_queued_count;
        inv_walk_queued_clicks[index] = ref;
        inv_walk_queued_count = index + 1;
    }
}

// Every outgoing packet (silent mode only).
extern "C" void inv_walk_on_gui_screen(GuiHook *hook) {
    if (inv_walk_mode != MODE_SILENT)
        return;

    void *packet = hook->screen;
    void *status_packet = java_jni_is_instance_of_c16_packet_client_status(hook->a, packet);
    void *jvm = hook->a;

    // "Open inventory" (status 2): hold it back. The server keeps thinking the inventory is closed.
    if (status_packet) {
        if (java_get_c16_packet_client_status_status(jvm, status_packet) == 2) {
            hook->handled = 1;
            inv_walk_phase = PHASE_SERVER_CLOSED;
        }
        return;
    }

    // Inventory click.
    if (java_jni_is_instance_of_c0_epacket_click_window(jvm, packet)) {
        if (inv_walk_phase == PHASE_SERVER_CLOSED) {
            // First click: open the inventory on the server now, and queue the click after it.
            java_call_generic_future_listener(hook->a, hook->screen_obj,
                                              java_new_c16_packet_client_status(hook->a, java_get_c16_enum_state_open_inventory(hook->a)));
            inv_walk_phase = PHASE_SERVER_OPEN;
            inv_walk_replaying_queued_clicks = 1;
            queue_packet(hook->a, packet);
            hook->handled = 1;
            return;
        }
        if (inv_walk_queued_count == 0 && !(inv_walk_replaying_queued_clicks & 1)) {
            // Nothing queued: let it through, and remember when it happened.
            inv_walk_tick_last_screen_activity = inv_walk_tick_counter;
            return;
        }
        // Other clicks are queued: keep the order and queue this one too.
        queue_packet(hook->a, packet);
        hook->handled = 1;
        return;
    }

    // "Close window".
    if (!java_jni_is_instance_of_c(hook->a, packet))
        return;
    if (inv_walk_phase == PHASE_SERVER_CLOSED) {
        // The server never saw the inventory open, so it does not need to see it close.
        hook->handled = 1;
        return;
    }
    if (inv_walk_queued_count) {
        // Must arrive after the queued clicks.
        queue_packet(hook->a, packet);
        hook->handled = 1;
    }
}

// Every tick: once the queued clicks have been sent, throw away the keyboard input
// that piled up meanwhile.
extern "C" void inv_walk_on_inventory_tick(Hook *hook) {
    if (inv_walk_replaying_queued_clicks)
        return;
    Ctx *ctx = hook->ctx;
    if (!release_global_refs(ctx->jvm, ctx->minecraft))
        return;
    inv_walk_tick_last_screen_activity = inv_walk_tick_counter;
    if (ctx->in_game != 1)
        return;
    while (keyboard_has_events(ctx->jvm))
        ;
    while (keyboard_has_next(ctx->jvm))
        ;
}

// True for "stop_duration" ticks after the last inventory click.
static bool clicked_recently() {
    return (unsigned)(inv_walk_tick_counter - inv_walk_tick_last_screen_activity) < (unsigned)(long long)inv_walk_stop_duration_value;
}

// Every tick: keep the movement keys pressed while a screen is open.
extern "C" void inv_walk_tick_keep_movement(Hook *hook) {
    Ctx *ctx = hook->ctx;
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    void *settings = get_game_settings(ctx->jvm, ctx->minecraft);
    void *back_key = java_get_game_settings_back_key(ctx->jvm, settings);
    void *left_key = java_get_game_settings_left_key(ctx->jvm, settings);
    void *right_key = java_get_game_settings_right_key(ctx->jvm, settings);
    void *forward_key = java_get_game_settings_forward_key(ctx->jvm, settings);
    void *jump_key = get_held_item(ctx->jvm, settings);

    // Which keys the player is physically holding.
    unsigned char back_held = java_get_key_binding_key_code(ctx->jvm, back_key);
    unsigned char left_held = java_get_key_binding_key_code(ctx->jvm, left_key);
    unsigned char right_held = java_get_key_binding_key_code(ctx->jvm, right_key);
    unsigned char forward_held = java_get_key_binding_key_code(ctx->jvm, forward_key);
    unsigned char jump_held = java_get_key_binding_key_code(ctx->jvm, jump_key);

    unsigned stop_on_click = inv_walk_stop_on_click;

    // cancel_sprint: stop sprinting this tick.  release_keys: force all movement keys up this tick.
    unsigned char cancel_sprint = (inv_walk_mode == MODE_SILENT && stop_on_click != STOP_NEVER) ? inv_walk_queued_count != 0 : 0;
    unsigned char release_keys = (stop_on_click == STOP_ALWAYS) & cancel_sprint;
    unsigned char update_keys;

    inv_walk_tick_counter += 1;

    if (!screen) {
        // No screen open: if one was open, give the keys back once.
        if (inv_walk_phase != PHASE_NO_SCREEN) {
            java_set_key_binding_pressed(ctx->jvm, back_key, back_held);
            java_set_key_binding_pressed(ctx->jvm, left_key, left_held);
            java_set_key_binding_pressed(ctx->jvm, right_key, right_held);
            java_set_key_binding_pressed(ctx->jvm, forward_key, forward_held);
            java_set_key_binding_pressed(ctx->jvm, jump_key, jump_held);
        }
        inv_walk_phase = PHASE_NO_SCREEN;
        if (!ctx->in_game)
            return;
        update_keys = 0;
    } else {
        unsigned char moving = back_held | left_held | right_held | forward_held | jump_held;
        unsigned char inventory_busy;

        if (!java_jni_is_instance_of_gui_inventory(ctx->jvm, screen)) {
            // A chest, furnace or other non-inventory screen.
            inv_walk_phase = PHASE_OTHER_SCREEN;
            if (inv_walk_mode == MODE_SILENT) {
                cancel_sprint |= moving ^ 1;
                unsigned char recent = 0;
                if (stop_on_click != STOP_NEVER)
                    recent = clicked_recently();
                unsigned char stop = cancel_sprint | recent;
                unsigned char stop_non_inv = (inv_walk_stop_non_inv_value != STOP_NEVER) | stop;
                cancel_sprint = stop_non_inv;
                if (((stop_on_click == STOP_ALWAYS) & stop) || ((inv_walk_stop_non_inv_value == STOP_ALWAYS) & stop_non_inv))
                    goto release_all;
            } else if (release_keys) {
                goto release_all;
            }
            goto press_held_keys;
        }

        // The player's own inventory.
        if (inv_walk_mode != MODE_SILENT) {
            inv_walk_phase = PHASE_SERVER_OPEN;
            update_keys = 1;
            inventory_busy = 0;
        } else {
            void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
            void *net_manager = java_get_net_handler_play_client_net_manager(ctx->jvm, java_call_minecraft_get_net_handler(ctx->jvm, ctx->minecraft));

            // The inventory is "busy" when an item is on the cursor or in the crafting grid.
            inventory_busy = 1;
            if (!java_get_inventory_player_item_stack(ctx->jvm, inventory)) {
                if (inv_walk_phase == PHASE_OTHER_SCREEN) {
                    inventory_busy = cancel_sprint;
                } else {
                    void *grid = java_get_container_player_craft_matrix(ctx->jvm, java_get_entity_player_inventory_container(ctx->jvm, ctx->player));
                    if (!java_call_iinventory_get_stack_in_slot(ctx->jvm, grid, 0) && !java_call_iinventory_get_stack_in_slot(ctx->jvm, grid, 1) &&
                        !java_call_iinventory_get_stack_in_slot(ctx->jvm, grid, 2) && !java_call_iinventory_get_stack_in_slot(ctx->jvm, grid, 3))
                        inventory_busy = cancel_sprint;
                }
            }
            moving &= inventory_busy ^ 1;

            int phase = inv_walk_phase;
            if (phase == PHASE_SERVER_CLOSED && !moving) {
                // Standing still now: tell the server the inventory is open.
                void *open_packet = java_new_c16_packet_client_status(ctx->jvm, java_get_c16_enum_state_open_inventory(ctx->jvm));
                java_call_generic_future_listener(ctx->jvm, net_manager, open_packet);
                inv_walk_phase = phase = PHASE_SERVER_OPEN;
            } else if (moving & (inv_walk_queued_count == 0) & (phase == PHASE_SERVER_OPEN)) {
                // Moving again and nothing queued: tell the server the inventory closed.
                if (stop_on_click == STOP_NEVER || !clicked_recently()) {
                    void *close_packet = java_new_c_init(ctx->jvm, 0);
                    java_call_generic_future_listener(ctx->jvm, net_manager, close_packet);
                    inv_walk_phase = phase = PHASE_SERVER_CLOSED;
                }
            }
            inventory_busy |= phase == PHASE_SERVER_OPEN;
            update_keys = phase != PHASE_NO_SCREEN;
        }
        cancel_sprint |= (stop_on_click != STOP_NEVER) & inventory_busy;
        release_keys = (stop_on_click == STOP_ALWAYS) & cancel_sprint;
    }

    // Nothing to do unless keys need updating or were forced up last tick.
    if (!((update_keys | inv_walk_keys_released_last_tick | release_keys) & 1))
        goto finish;
    if (!release_keys)
        goto press_held_keys;

release_all:
    java_set_key_binding_pressed(ctx->jvm, back_key, 0);
    java_set_key_binding_pressed(ctx->jvm, left_key, 0);
    java_set_key_binding_pressed(ctx->jvm, right_key, 0);
    java_set_key_binding_pressed(ctx->jvm, forward_key, 0);
    java_set_key_binding_pressed(ctx->jvm, jump_key, 0);
    release_keys = 1;
    goto finish;

press_held_keys:
    java_set_key_binding_pressed(ctx->jvm, back_key, back_held);
    java_set_key_binding_pressed(ctx->jvm, left_key, left_held);
    java_set_key_binding_pressed(ctx->jvm, right_key, right_held);
    java_set_key_binding_pressed(ctx->jvm, forward_key, forward_held);
    java_set_key_binding_pressed(ctx->jvm, jump_key, jump_held);
    release_keys = 0;

finish:
    if (cancel_sprint) {
        java_call_entity_set_sprinting(ctx->jvm, ctx->player, false);
        void *jvm = ctx->jvm;
        java_set_key_binding_pressed(jvm, java_get_game_settings_key_17_or_8(jvm, settings), 0);
    }
    inv_walk_keys_released_last_tick = release_keys;
    inv_walk_replaying_queued_clicks = 0;
}
