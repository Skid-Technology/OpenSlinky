// Inv Manager - sort your inventory automatically while it is open.
//
// For each armour slot and each hotbar slot, the settings say which kind of item belongs
// there (sword, blocks, bow, ...). While the inventory is open, the module finds the best
// such item, and clicks it into place: shift-click for armour, "number key" swaps for the
// hotbar, and pick-up / put-down for merging stacks. Clicks are spaced by "click_delay",
// longer for longer mouse moves ("scale_by_mouse_distance") and at random ("randomise").
//
// Functions:
//   inv_manager_score_item_stack  how good an item is for a slot
//   inv_manager_tick              every tick: the next click
//   inv_manager_plan_step         helper: find the best item for one hotbar slot
#include "slinky/engine.h"
#include "slinky/jids.h"

// ---- settings ----
extern "C" unsigned char inv_manager_try_paired_slot;   // "swap_sides": also try the other hand's / side's kind
extern "C" char inv_manager_enum_slot_1[];               // per hotbar slot settings, 0x40 bytes each (+0x30 wanted kind)
extern "C" float inv_manager_start_delay;                // ticks after opening the inventory
extern "C" float inv_manager_click_delay_b, inv_manager_click_delay_b_second;   // click delay = first / second
extern "C" unsigned char inv_manager_randomise;
extern "C" unsigned char inv_manager_scale_by_mouse_distance;

// ---- game / engine functions ----
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(int, java_get_item_stack_metadata, (void *jvm, void *stack));
FN(void *, java_call_item_stack_get_enchantment_tag_list, (void *jvm, void *stack));
FN(unsigned char, java_call_item_stack_has_display_name, (void *jvm, void *stack));
FN(unsigned short, score_item_inventory_sorter_category, (void **s, int slot, int id, int dmg, void *nbt, unsigned char flag));   // score for a kind (0 = does not fit)

// How good the item stack is as kind `kind`. With "swap_sides", an item that does not fit
// may still score (lower) as the paired kind.
extern "C" unsigned inv_manager_score_item_stack(void **s, void *stack, int kind) {
    void *item = java_call_item_stack_get_item(*s, stack);
    if (!item)
        return 0;
    int item_id = java_call_static_item_get_id_from_item(*s, item);
    int damage = java_get_item_stack_metadata(*s, stack);
    void *enchantments = java_call_item_stack_get_enchantment_tag_list(*s, stack);
    unsigned char named = java_call_item_stack_has_display_name(*s, stack);
    unsigned score = score_item_inventory_sorter_category(s, kind, item_id, damage, enchantments, named);
    if (inv_manager_try_paired_slot != 1)
        return score;
    if (score)
        return score << 16;   // a real fit always beats a paired one
    int paired;
    switch (kind) {
    case 22: paired = 25; break;
    case 17: paired = 7; break;
    case 7: paired = 17; break;
    default: return 0;
    }
    return score_item_inventory_sorter_category(s, paired, item_id, damage, enchantments, named);
}

struct Pair { float x, y; };
struct Range { float lo, hi; };

// The module's progress (all in one block of memory).
struct InvState {
    int state;                      // what we are doing (see below)
    int padding_04;
    unsigned long long mask;        // targets already handled
    int action;                     // the planned click: 0 shift-click, 1 swap with hotbar `slot`, 2 pick up and put in `slot`
    unsigned char slot, padding_15, padding_16, padding_17;
    int saved;                      // state to go back to after the click
    unsigned char sub;              // current target within the state
    unsigned char window;           // window slot to click
    unsigned char advance;          // go to the next target after the click
    unsigned char padding_1f;
    float delay;                    // ticks until the next click
    int padding_24;
    Pair pos;                       // where the "mouse" is (for the delay)
};
enum InvStep {
    STEP_ARMOUR = 0,      // plan the armour slots
    STEP_HOTBAR = 1,      // plan the hotbar slots
    STEP_DONE = 2,
    STEP_CLICK = 3,       // do the planned click
    STEP_PUT_DOWN = 4,    // (pick up action) put the stack down
    STEP_DROP_REST = 5,   // drop what is left on the cursor
};

struct InvHook {
    char padding_00[8];
    Ctx *ctx;               // +0x08 game context
    unsigned char busy;     // +0x10 set to 1: we use the inventory this tick
};

// A planning function for one target: true when it planned a click.
typedef unsigned char (*Step)(Ctx *c, InvState *st);

extern "C" InvState inv_manager_rotation_walk;                    // the progress
extern "C" unsigned char *const inv_manager_slot_setting_ptrs[2]; // armour / hotbar part enabled
extern "C" const unsigned char inv_manager_targets_per_state[2];  // number of targets: armour, hotbar
extern "C" const Step ptr_to_plan_inventory_swap[2];              // planning function: armour, hotbar (inv_manager_plan_step)

FN(void *, get_current_screen, (void *jvm, void *object));
FN(unsigned char, java_jni_is_instance_of_gui_inventory, (void *jvm, void *g));
FN(void, java_gui_container_handle_mouse_click, (void *jvm, void *g, int slot, int button, int mode));
FN(Pair, slot_index, (int slot));                     // screen position of a window slot
FN(float, rot_delta_length, (Pair a, Pair b));        // distance between two positions
FN(Range, rot_make_floats, (float lo, float hi));
FN(float, random_rotation, (Range r));                // random value in the range
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_get_inventory_player_main_inventory, (void *jvm, void *inventory_player));
FN(int, java_get_item_stack_stack_size, (void *jvm, void *stack));
FN(int, java_call_item_stack_get_max_stack_size, (void *jvm, void *stack));
FN(unsigned char, java_call_static_item_stack_are_item_stack_tags_equal, (void *jvm, void *item_stack, void *other_object));

// Move the "mouse" to `target`: wait the click delay (scaled by the distance / randomly).
static void move_to(Pair target) {
    float delay = inv_manager_click_delay_b / inv_manager_click_delay_b_second;
    if (inv_manager_scale_by_mouse_distance == 1) {
        float distance = rot_delta_length(target, inv_manager_rotation_walk.pos) * 0.5f;
        delay *= distance > 1.0f ? distance : 1.0f;
    }
    if (inv_manager_randomise == 1)
        delay *= random_rotation(rot_make_floats(1.0f, 1.25f));
    inv_manager_rotation_walk.delay = delay + inv_manager_rotation_walk.delay;
    inv_manager_rotation_walk.pos = target;
}

extern "C" void inv_manager_tick(InvHook *hook) {
    InvState &st = inv_manager_rotation_walk;
    Ctx *ctx = hook->ctx;

    // Inventory not open: get ready for next time (start delay, mouse in the middle).
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (!screen || !java_jni_is_instance_of_gui_inventory(ctx->jvm, screen)) {
        __builtin_memset(&st, 0, 0x20);
        st.delay = inv_manager_start_delay;
        if (inv_manager_randomise == 1)
            st.delay = random_rotation(rot_make_floats(1.0f, 1.5f)) * st.delay;
        st.pos = slot_index(13);
        return;
    }

    int state = st.state;
    if (state == STEP_DONE)
        return;
    if (st.delay > 0.0f) {
        st.delay += -1.0f;
        if (st.delay > 0.0f)
            return;
    }
    hook->busy = 1;

    int button = 0, mode = 0;
    for (;;) {
        switch (state) {
        case STEP_ARMOUR:
        case STEP_HOTBAR:
            // Plan the next target that needs a click.
            if (*inv_manager_slot_setting_ptrs[state]) {
                for (unsigned char targets = inv_manager_targets_per_state[state]; st.sub < targets; st.sub++) {
                    if (ptr_to_plan_inventory_swap[state](ctx, &st)) {
                        move_to(slot_index(st.window));
                        st.state = STEP_CLICK;
                        st.saved = state;
                        st.advance = state != STEP_HOTBAR;
                        return;
                    }
                }
            }
            // All done here: next part (clear everything but the state).
            st.state = ++state;
            __builtin_memset((char *)&st + 4, 0, 0x1c);
            continue;

        case STEP_CLICK: {
            int action = st.action;
            unsigned char slot = st.slot;
            if (action == 2) {          // pick up
                button = 0;
                mode = 0;
            } else if (action == 1) {   // swap with hotbar slot (like pressing its number key)
                button = slot;
                mode = 2;
            } else if (action == 0) {   // shift-click
                button = 0;
                mode = 1;
            }
            java_gui_container_handle_mouse_click(ctx->jvm, screen, st.window, button, mode);
            st.mask |= (long long)(1 << (st.sub & 31));
            if (st.advance == 1)
                st.sub++;
            if (action == 2) {
                // Then put it down in `slot`.
                move_to(slot_index(slot));
                st.state = STEP_PUT_DOWN;
                st.window = slot;
                return;
            }
            state = st.state = st.saved;
            continue;
        }

        case STEP_PUT_DOWN: {
            java_gui_container_handle_mouse_click(ctx->jvm, screen, st.window, 0, 0);
            // Then move the mouse out of the window, to drop what did not fit.
            Pair outside = st.pos;
            outside.x += -2.0f;
            move_to(outside);
            st.state = STEP_DROP_REST;
            return;
        }

        case STEP_DROP_REST:
            java_gui_container_handle_mouse_click(ctx->jvm, screen, -999, 0, 0);   // click outside the window
            state = st.state = st.saved;
            continue;

        default:
            return;
        }
    }
}

// An item's id and metadata as one number (0 for no item).
static unsigned long long item_key(Ctx *ctx, void *stack) {
    void *item = java_call_item_stack_get_item(ctx->jvm, stack);
    if (!item)
        return 0;
    unsigned item_id = java_call_static_item_get_id_from_item(ctx->jvm, item);
    return ((unsigned long long)item_id << 32) | (unsigned long long)(long long)java_get_item_stack_metadata(ctx->jvm, stack);
}

// Plan hotbar slot `st->sub`: find the stack that is best there (score x count), or one that
// can be merged into the current stack. True when a click was planned.
extern "C" unsigned char inv_manager_plan_step(Ctx *ctx, InvState *st) {
    unsigned target = st->sub;
    int wanted_kind = *(int *)(inv_manager_enum_slot_1 + target * 0x40 + 0x30);
    if (!wanted_kind)
        return 0;   // "any": leave it alone

    void *items = java_get_inventory_player_main_inventory(ctx->jvm, java_get_entity_player_inventory(ctx->jvm, ctx->player));
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    auto get = (*env)->GetObjectArrayElement;

    // What is there now.
    void *current = get(env, items, target);
    int count = 0, max_count = 0;
    unsigned long long key = 0;
    unsigned score = 0;
    if (current) {
        count = java_get_item_stack_stack_size(ctx->jvm, current);
        max_count = java_call_item_stack_get_max_stack_size(ctx->jvm, current);
        key = item_key(ctx, current);
        score = inv_manager_score_item_stack((void **)ctx, current, wanted_kind);
    }

    // Is there an empty slot in the main inventory (9..35)? (empty_slot == null)
    void *empty_slot;
    int i = 9;
    for (;;) {
        empty_slot = get(env, items, i);
        if (!empty_slot || i == 35)
            break;
        i++;
    }

    if (count <= 0)
        count = 0;
    unsigned best = count * score;
    unsigned best_slot = target;
    unsigned char merge = 0;
    for (unsigned j = 0; j < 36; j++) {
        if (j == target || (st->mask >> j & 1))
            continue;
        void *stack = get(env, items, j);
        if (!stack)
            continue;
        int n = java_get_item_stack_stack_size(ctx->jvm, stack);
        unsigned long long other_key = item_key(ctx, stack);

        // Value when merged into the current stack (same item, both not full).
        unsigned merged = 0;
        if (count < max_count && n < max_count && other_key == key && java_call_static_item_stack_are_item_stack_tags_equal(ctx->jvm, current, stack)) {
            int total = count + n;
            merged = (total < max_count ? total : max_count) * score;
        }
        unsigned value = inv_manager_score_item_stack((void **)ctx, stack, wanted_kind) * n;
        bool use_merge = value - 1 < merged;   // (value > 0 and merging is worth at least as much)
        if (use_merge)
            value = merged;
        // Merging from the hotbar only when there is room for the rest.
        if (value > best && (!empty_slot || j >= 9 || !use_merge)) {
            best = value;
            best_slot = j;
            merge = use_merge;
        }
    }

    if (best_slot != target) {
        st->padding_15 = st->padding_16 = st->padding_17 = 0;
        st->action = !merge;                  // merge: shift-click (0), else swap into the hotbar slot (1)
        st->slot = merge ? 0 : target;
        st->window = (int)best_slot >= 9 ? best_slot : best_slot + 36;   // hotbar slots are 36..44 in the window
        st->mask |= (long long)(1 << (target & 31));
        return 1;
    }
    if (best)
        st->mask |= (long long)(1 << (target & 31));
    return 0;
}
