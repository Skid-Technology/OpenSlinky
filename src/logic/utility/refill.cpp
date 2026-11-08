// Refill - fill the empty hotbar slots with soup / healing potions from the inventory.
//
// The module opens your inventory, shift-clicks soups (and with "refill_potion" splash
// potions) from the inventory into the hotbar until it is full, then closes the inventory
// and switches itself off. Clicks are spaced like a human's: slower for slots further away
// on screen ("distance_based_delay"), with random variation.
//
// Callbacks:
//   refill_enable              switched on: start over
//   refill_classify_27_main    helper: sort the 27 inventory slots by what they hold
//   refill_on_inventory_tick   every tick: the next step
#include "slinky/engine.h"

struct Range { float lo, hi; };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

struct InvHook {
    char padding_00[8];
    Ctx *ctx;                // +0x08 game context
    unsigned char handled;   // +0x10 set to 1: we use the inventory screen this tick
};

// ---- settings ----
extern "C" unsigned char refill_refill_potion;          // also move splash potions
extern "C" float refill_open_delay_setting;             // "open_delay": ticks after opening
extern "C" float refill_close_delay_setting;            // "close_delay": ticks before closing
extern "C" float refill_speed_value;                    // "speed"
extern "C" unsigned char refill_distance_based_delay;   // slower for slots further away
extern "C" unsigned refill_click_button_mode;           // mouse button: 0 left, 1 right, 2 random

// ---- module state ----
extern "C" char refill_module_descriptor[];
// Slot class numbers (see refill_classify_27_main).
extern "C" unsigned char refill_soup_class_offset;
extern "C" unsigned char refill_non_health_potion_class_offset;
extern "C" unsigned char refill_slot_classes[27];       // class of each inventory slot (9..35)

// What to do next.
enum RefillStep {
    STEP_OPEN = 0,          // open the inventory
    STEP_START = 1,         // inventory open: pick where the "mouse" starts
    STEP_CHOOSE_SLOT = 2,   // pick the next slot to move
    STEP_CLICK = 3,         // shift-click it
    STEP_DONE = 4,          // close the inventory and switch off
};
extern "C" int refill_pending_action;

extern "C" float refill_step_delay;            // ticks until the next step
extern "C" int refill_last_slot;               // the slot the "mouse" is on
extern "C" float refill_cell_angle;            // preferred direction to the next slot
extern "C" int refill_slot_being_moved;
extern "C" int refill_random_button;
extern "C" char refill_open_delay_timer[], refill_close_delay_timer[], refill_random_walk_1[], refill_random_walk_2[], refill_gauss_rng_state[], refill_random_walk_3[];   // random states

// ---- game / engine functions ----
FN(void *, get_current_screen, (void *jvm, void *object));
FN(unsigned char, java_jni_is_instance_of_gui_inventory, (void *jvm, void *screen));
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, walk_update, (void *state, Range r, double p));            // next value of a random walk
FN(unsigned char, walk_should_change, (void *state, Range r, double p));   // random yes/no
FN(float, random_rotation, (Range r));                                // random value in the range
FN(float, gauss_random, (void *st));                                  // normally distributed random number
FN(int, refill_pick_inventory_cell, (int last, float bias, float *dist));   // next slot to move (or -1), and its distance
FN(void, refill_disable_and_close_screen, (Ctx *ctx));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(void *, get_game_screen, (void *jvm, void *object));               // here: the inventory key
FN(void, java_increment_key_binding_press_time, (void *jvm, void *key_binding));
FN(void, java_gui_container_handle_mouse_click, (void *jvm, void *screen, int slot, int button, int mode));   // mode 1 = shift-click
FN(void, queue_module_toggle, (void *module_manager, void *module, int enabled));
FN(void, keyboard_send_key, (void *jvm, void *screen, int x, int y));   // (0, 1) = escape: close the screen
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *inv, int slot));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(unsigned, java_get_item_stack_metadata, (void *jvm, void *stack));

enum { ITEM_MUSHROOM_STEW = 282, ITEM_POTION = 373, POTION_SPLASH_FLAG = 0x4000, POTION_EFFECT_HEALING = 5 };

extern "C" void refill_enable(EvHook *hook) {
    if (hook->ev != refill_module_descriptor || hook->on != 1)
        return;
    refill_pending_action = STEP_OPEN;
}

// Sort the 27 inventory slots (9..35) by what they hold:
//   0 = empty, 1 = something else, 2 = splash healing potion,
//   refill_non_health_potion_class_offset + 1 = another splash potion, refill_soup_class_offset + 1 = soup
extern "C" void refill_classify_27_main(Ctx *ctx) {
    void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    for (long i = 0; i != 27; i++) {
        unsigned char slot_class = 0;
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, (int)i + 9);
        if (stack) {
            void *jvm = ctx->jvm;
            void *item = java_call_item_stack_get_item(jvm, stack);
            int item_id = item ? java_call_static_item_get_id_from_item(jvm, item) : 0;
            if (item_id) {
                switch (item_id) {
                case ITEM_POTION:
                    slot_class = 1;
                    if (refill_refill_potion == 1) {
                        unsigned damage = java_get_item_stack_metadata(jvm, stack);
                        if (damage & POTION_SPLASH_FLAG) {
                            slot_class = 2;
                            if ((damage & 0xf) != POTION_EFFECT_HEALING)
                                slot_class = refill_non_health_potion_class_offset + 1;
                        }
                    }
                    break;
                case ITEM_MUSHROOM_STEW:
                    slot_class = refill_soup_class_offset + 1;
                    break;
                default:
                    slot_class = 1;
                    break;
                }
            }
        }
        refill_slot_classes[i] = slot_class;
    }
}

// Wait `base` ticks, minus up to 2 at random.
static void add_delay(float base, char *random_state) {
    double delay = (double)base - (double)walk_update(random_state, rot_make_floats(0.0f, 2.0f), 30.0);
    delay = delay > 0.0 ? delay : 0.0;
    refill_step_delay = (float)((double)refill_step_delay + delay);
}

// Is there an empty slot in the hotbar?
static bool hotbar_has_space(Ctx *ctx) {
    void *inventory = java_get_entity_player_inventory(ctx->jvm, ctx->player);
    for (int i = 0; i < 9; i++) {
        void *stack = java_call_iinventory_get_stack_in_slot(ctx->jvm, inventory, i);
        if (!stack)
            return true;
        void *item = java_call_item_stack_get_item(ctx->jvm, stack);
        if (!item || !java_call_static_item_get_id_from_item(ctx->jvm, item))
            return true;
    }
    return false;
}

// Switch off and close the screen.
static void finish(Ctx *ctx) {
    queue_module_toggle(ctx->module_manager, refill_module_descriptor, 0);
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (screen)
        keyboard_send_key(ctx->jvm, screen, 0, 1);
}

extern "C" void refill_on_inventory_tick(InvHook *hook) {
    Ctx *ctx = hook->ctx;
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (refill_pending_action != STEP_OPEN) {
        // The player closed the inventory: stop.
        if (!screen || !java_jni_is_instance_of_gui_inventory(ctx->jvm, screen)) {
            finish(ctx);
            return;
        }
        hook->handled = 1;
    }

    // Wait for the delay.
    float delay = refill_step_delay;
    if (delay > 0.0f) {
        refill_step_delay = delay + -1.0f;
        return;
    }
    if (!(0.0f >= delay))
        return;

    // Do steps until one needs a delay.
    int button = 0;
    for (;;) {
        if (refill_pending_action == STEP_DONE) {
            finish(ctx);
            return;
        }
        if (!ctx->player || !hotbar_has_space(ctx))
            break;   // hotbar full: done

        switch (refill_pending_action) {
        case STEP_OPEN: {
            refill_pending_action = STEP_START;
            if (screen)
                return;
            refill_classify_27_main(ctx);
            float distance;
            if (refill_pick_inventory_cell(refill_last_slot, refill_cell_angle, &distance) == -1) {
                refill_disable_and_close_screen(ctx);   // nothing to move
                return;
            }
            java_increment_key_binding_press_time(ctx->jvm, get_game_screen(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft)));   // open the inventory
            return;
        }
        case STEP_START:
            refill_last_slot = (int)random_rotation(rot_make_floats(0.0f, 27.0f));
            refill_cell_angle = random_rotation(rot_make_floats(0.0f, 3.14159265f));
            refill_pending_action = STEP_CHOOSE_SLOT;
            add_delay(refill_open_delay_setting, refill_open_delay_timer);
            break;
        case STEP_CHOOSE_SLOT: {
            float distance;
            int slot = refill_pick_inventory_cell(refill_last_slot, refill_cell_angle, &distance);
            refill_slot_being_moved = slot;
            if (slot == -1) {
                add_delay(refill_close_delay_setting, refill_close_delay_timer);
                refill_pending_action = STEP_DONE;
                return;
            }
            // Time to move the mouse there: distance / speed, with random variation
            // (+-20% drift, gaussian 30%, now and then 1.4-1.8x slower).
            float speed = walk_update(refill_random_walk_1, rot_make_floats(0.8f, 1.2f), 30.0) * refill_speed_value;
            speed = (float)((double)speed * ((double)gauss_random(refill_gauss_rng_state) * 0.3 + 1.0));
            if (walk_should_change(refill_random_walk_2, rot_make_floats(0.04f, 0.08f), 30.0))
                speed /= random_rotation(rot_make_floats(1.4f, 1.8f));
            float how_far = refill_distance_based_delay ? distance : 1.0f;
            refill_step_delay = how_far / speed + refill_step_delay;
            refill_last_slot = refill_slot_being_moved;
            refill_pending_action = STEP_CLICK;
            break;
        }
        case STEP_CLICK: {
            unsigned mode = refill_click_button_mode;
            if (mode < 2) {
                button = (int)mode;
            } else if (mode == 2) {
                button = refill_random_button;
                if (walk_should_change(refill_random_walk_3, rot_make_floats(0.9f, 1.0f), 10.0))
                    refill_random_button = refill_random_button == 0;
            }
            java_gui_container_handle_mouse_click(ctx->jvm, screen, refill_slot_being_moved + 9, button, 1);
            refill_slot_classes[refill_slot_being_moved] = 0;
            refill_pending_action = STEP_CHOOSE_SLOT;
            break;
        }
        }
        if (!(0.0f >= refill_step_delay))
            return;
    }

    // Hotbar full: close after "close_delay".
    add_delay(refill_close_delay_setting, refill_close_delay_timer);
    refill_pending_action = STEP_DONE;
    if (!(0.0f >= refill_step_delay))
        return;
    finish(ctx);
}
