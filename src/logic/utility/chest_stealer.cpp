// Chest Stealer - take the useful items out of a chest.
//
// While a chest is open, the module shift-clicks the items worth taking, one per click
// ("speed" clicks per 50 ms), after "delay_after_open" ms:
//   - tools, weapons and armour that are better than what you have,
//   - golden apples ("Golden apples") and useful potions ("Good potions"),
//   - building blocks, up to "Max blocks" in your inventory.
// With "require_vanilla_title", only chests with the normal "Chest" / "Large Chest" title.
//
// Callback (descriptor slot +0x1d8):
//   chest_stealer_on_tick  every tick
#include "slinky/engine.h"
#include "slinky/jids.h"

S(str_container_chest);  // "container.chest"
S(str_container_chestdouble);  // "container.chestDouble"

extern "C" float chest_stealer_delay_after_open_value;                 // "delay_after_open" current value
extern "C" unsigned char chest_stealer_require_vanilla_title_value;         // "require_vanilla_title"
extern "C" float chest_stealer_steal_delay;                 // steal delay (ms)
extern "C" unsigned char chest_stealer_speed_setting[];     // slider; current value at +0x30
extern "C" unsigned char chest_stealer_max_blocks_setting[];  // slider; current value at +0x30
extern "C" unsigned char chest_stealer_golden_apples_value;     // take golden apples
extern "C" unsigned char chest_stealer_good_potions_value;    // take useful potions

struct ToolInfo { int kind, tier; };
FN(void *, get_current_screen, (void *jvm, void *object));
FN(unsigned char, java_jni_is_instance_of_gui_chest, (void *jvm, void *scr));
FN(void *, java_get_gui_chest_lower_chest_inventory, (void *jvm, void *scr));
FN(void *, java_jni_is_instance_of_inventory_basic, (void *jvm, void *candidate));
FN(void *, java_get_inventory_basic, (void *jvm, void *inventory_basic));
FN(unsigned char, java_string_equal_translation_key, (void *jvm, void *title, const char *name));
FN(void *, java_call_iinventory_get_stack_in_slot, (void *jvm, void *inventory, int slot));
FN(void, java_gui_container_handle_mouse_click, (void *jvm, void *screen, int slot, int button, int mode));
FN(void *, java_get_entity_player_inventory, (void *jvm, void *entity_player));
FN(void *, java_call_item_stack_get_item, (void *jvm, void *stack));
FN(int, java_call_static_item_get_id_from_item, (void *jvm, void *item));
FN(int, java_get_item_stack_metadata, (void *jvm, void *stack));
FN(int, java_get_item_stack_stack_size, (void *jvm, void *stack));
FN(void *, java_call_item_stack_get_enchantment_tag_list, (void *jvm, void *stack));
FN(unsigned char, java_call_item_stack_has_display_name, (void *jvm, void *stack));
FN(int, score_item_inventory_sorter_category, (Ctx *ctx, int category, int id, unsigned meta, void *nbt, unsigned char display_name));
FN(ToolInfo, chest_gui_title_id, (int id));
FN(unsigned, swap_delay, (Ctx *ctx, void *stack, unsigned kind));
FN(double, time_now, ());

static void *chest_stealer_screen;          // screen whose slots are being processed
static int chest_stealer_slot;              // next chest slot to inspect
static int chest_stealer_size;              // number of slots in the lower chest inventory
static double chest_stealer_next_click;     // time of the next click
static jmethodID chest_stealer_size_method; // IInventory.getSizeInventory

static jmethodID inventory_size_method(Jvm *jvm) {
    if (chest_stealer_size_method) return chest_stealer_size_method;
    JNIEnv *env = jvm->env;
    jclass cls = (jclass)jvm->ids->IInventory;
    static const char *const names[] = {"getSizeInventory", "func_70302_i_", "a"};
    for (int i = 0; i < 3; i++) {
        jmethodID id = (*env)->GetMethodID(env, cls, names[i], "()I");
        if (id) {
            chest_stealer_size_method = id;
            return id;
        }
        (*env)->ExceptionClear(env);
    }
    return 0;
}

static float speed_value() {
    float s = *(float *)(chest_stealer_speed_setting + 0x30);
    if (!(s > 0.1f) || s > 10.0f) s = 1.0f;
    return s;
}

// Score a tool against the sorter's tool category. kind is 0 for non-tools.
static int tool_score(Ctx *ctx, void *stack, int *kind) {
    Jvm *jvm = (Jvm *)ctx->jvm;
    JNIEnv *env = jvm->env;
    jobject item = java_call_item_stack_get_item(jvm, stack);
    if (!item) return -1;
    int id = java_call_static_item_get_id_from_item(jvm, item);
    ToolInfo info = chest_gui_title_id(id);
    if (!info.kind) {
        (*env)->DeleteLocalRef(env, item);
        return -1;
    }
    unsigned meta = (unsigned)java_get_item_stack_metadata(jvm, stack);
    void *nbt = java_call_item_stack_get_enchantment_tag_list(jvm, stack);
    unsigned char display_name = java_call_item_stack_has_display_name(jvm, stack);
    int score = score_item_inventory_sorter_category(ctx, info.kind, id, meta, nbt, display_name);
    if (nbt) (*env)->DeleteLocalRef(env, nbt);
    (*env)->DeleteLocalRef(env, item);
    *kind = info.kind;
    return score;
}

static int max_block_count() {
    float value = *(float *)(chest_stealer_max_blocks_setting + 0x30);
    if (!(value > 0.0f)) return 0;
    if (value > 2304.0f) value = 2304.0f;   // 36 inventory slots * 64
    return (int)value;
}

static bool is_block_item(int id) {
    return id > 0 && id < 0x100;
}

// 1.8 potion damage values: the low nibble is the base potion, bit 0x20 is
// level II, bit 0x40 is extended, and bit 0x4000 marks a splash potion.
static bool is_good_potion(unsigned meta) {
    switch (meta & 0xf) {
    case 1:  // regeneration
    case 2:  // swiftness
    case 3:  // fire resistance
    case 5:  // instant health
    case 6:  // night vision
    case 8:  // strength
    case 10: // jump boost
    case 12: // water breathing
    case 13: // invisibility
        return true;
    default:
        return false;
    }
}

static int player_block_count(Ctx *ctx) {
    Jvm *jvm = (Jvm *)ctx->jvm;
    JNIEnv *env = jvm->env;
    void *inventory = java_get_entity_player_inventory(jvm, ctx->player);
    if (!inventory) return 0;
    int total = 0;
    for (int slot = 0; slot < 36 && total < 2304; slot++) {
        void *current = java_call_iinventory_get_stack_in_slot(jvm, inventory, slot);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            break;
        }
        if (!current) continue;
        jobject item = java_call_item_stack_get_item(jvm, current);
        if (item) {
            int id = java_call_static_item_get_id_from_item(jvm, item);
            if (is_block_item(id)) {
                int n = java_get_item_stack_stack_size(jvm, current);
                if (n > 0) total += n;
            }
            (*env)->DeleteLocalRef(env, item);
        }
        (*env)->DeleteLocalRef(env, current);
    }
    (*env)->DeleteLocalRef(env, inventory);
    return total;
}

static bool chest_block_is_wanted(Ctx *ctx, void *stack, int current_blocks) {
    int limit = max_block_count();
    if (limit <= 0) return false;
    Jvm *jvm = (Jvm *)ctx->jvm;
    int n = java_get_item_stack_stack_size(jvm, stack);
    if (n <= 0) return false;
    return current_blocks + n <= limit;
}

// Golden apples, useful potions, and a configurable number of building blocks.
static bool chest_item_is_special(Ctx *ctx, void *stack, int blocks_in_inventory) {
    Jvm *jvm = (Jvm *)ctx->jvm;
    JNIEnv *env = jvm->env;
    jobject item = java_call_item_stack_get_item(jvm, stack);
    if (!item) return false;
    int id = java_call_static_item_get_id_from_item(jvm, item);
    bool wanted = false;
    if (id == 0x142 || id == 0x1d2) {           // golden / enchanted golden apple
        wanted = chest_stealer_golden_apples_value == 1;
    } else if (id == 0x175) {                    // potion
        wanted = chest_stealer_good_potions_value == 1 &&
                 is_good_potion((unsigned)java_get_item_stack_metadata(jvm, stack));
    } else if (is_block_item(id)) {
        wanted = chest_block_is_wanted(ctx, stack, blocks_in_inventory);
    }
    (*env)->DeleteLocalRef(env, item);
    return wanted;
}

// True only when the chest stack is an upgrade over the player's current gear.
static bool chest_item_is_upgrade(Ctx *ctx, void *stack) {
    Jvm *jvm = (Jvm *)ctx->jvm;
    JNIEnv *env = jvm->env;

    int kind = 0;
    int chest_score = tool_score(ctx, stack, &kind);
    void *inventory = java_get_entity_player_inventory(jvm, ctx->player);
    if (inventory && kind) {
        int best = 0;
        for (int slot = 0; slot < 36; slot++) {
            void *current = java_call_iinventory_get_stack_in_slot(jvm, inventory, slot);
            if (!current) continue;
            int current_kind = 0;
            int current_score = tool_score(ctx, current, &current_kind);
            if (current_kind == kind && current_score > best) best = current_score;
            (*env)->DeleteLocalRef(env, current);
        }
        (*env)->DeleteLocalRef(env, inventory);
        return chest_score > best;
    }

    if (inventory) {
        // Armor: kind 0 helmet, 1 chestplate, 2 leggings, 3 boots. swap_delay returns
        // armour points plus protection enchantments for the matching piece.
        for (int armor_kind = 0; armor_kind < 4; armor_kind++) {
            unsigned upgrade_score = swap_delay(ctx, stack, (unsigned)armor_kind);
            if (!upgrade_score) continue;
            void *current = java_call_iinventory_get_stack_in_slot(jvm, inventory, 39 - armor_kind);
            unsigned current_score = current ? swap_delay(ctx, current, (unsigned)armor_kind) : 0;
            if (current) (*env)->DeleteLocalRef(env, current);
            if (upgrade_score > current_score) {
                (*env)->DeleteLocalRef(env, inventory);
                return true;
            }
        }
        (*env)->DeleteLocalRef(env, inventory);
    }
    return false;
}

extern "C" void chest_stealer_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    Jvm *jvm = (Jvm *)ctx->jvm;
    void *screen = get_current_screen(ctx->jvm, ctx->minecraft);
    if (!screen || !java_jni_is_instance_of_gui_chest(ctx->jvm, screen)) {
        chest_stealer_screen = 0;
        chest_stealer_slot = 0;
        return;
    }

    void *lower = java_get_gui_chest_lower_chest_inventory(ctx->jvm, screen);
    if (!lower) return;

    // "require_vanilla_title" means: only click chests whose localized title is the
    // vanilla chest title.
    if (chest_stealer_require_vanilla_title_value == 1) {
        void *inventory_basic = java_jni_is_instance_of_inventory_basic(jvm, lower);
        if (inventory_basic) {
            void *title = java_get_inventory_basic(jvm, inventory_basic);
            if (!java_string_equal_translation_key(jvm, title, str_container_chest) &&
                !java_string_equal_translation_key(jvm, title, str_container_chestdouble))
                return;
        }
    }

    double now = time_now();
    if (screen != chest_stealer_screen) {
        chest_stealer_screen = screen;
        chest_stealer_slot = 0;
        chest_stealer_size = 0;
        chest_stealer_next_click = now + (double)chest_stealer_delay_after_open_value / 1000.0;
    }
    if (now < chest_stealer_next_click) return;

    if (chest_stealer_size <= 0) {
        jmethodID size_method = inventory_size_method(jvm);
        if (size_method) {
            chest_stealer_size = (*jvm->env)->CallIntMethod(jvm->env, lower, size_method);
            if ((*jvm->env)->ExceptionCheck(jvm->env)) {
                (*jvm->env)->ExceptionClear(jvm->env);
                chest_stealer_size = 0;
            }
        }
        // If the size lookup is unavailable, use the single-chest size. Never probe
        // past it, or a single chest would start moving the player's own items into it.
        if (chest_stealer_size <= 0 || chest_stealer_size > 54) chest_stealer_size = 27;
    }

    int blocks_in_inventory = max_block_count() > 0 ? player_block_count(ctx) : 0;
    for (; chest_stealer_slot < chest_stealer_size; chest_stealer_slot++) {
        void *stack = java_call_iinventory_get_stack_in_slot(jvm, lower, chest_stealer_slot);
        if ((*jvm->env)->ExceptionCheck(jvm->env)) {
            (*jvm->env)->ExceptionClear(jvm->env);
            return;
        }
        if (!stack) continue;
        bool take = chest_item_is_special(ctx, stack, blocks_in_inventory);
        if (!take) take = chest_item_is_upgrade(ctx, stack);
        (*jvm->env)->DeleteLocalRef(jvm->env, stack);
        if (!take) continue;
        java_gui_container_handle_mouse_click(jvm, screen, chest_stealer_slot, 0, 1);   // shift-click
        if ((*jvm->env)->ExceptionCheck(jvm->env)) {
            (*jvm->env)->ExceptionClear(jvm->env);
            return;
        }
        chest_stealer_steal_delay = (float)(0.05 / (double)speed_value() * 1000.0);
        chest_stealer_slot++;
        chest_stealer_next_click = now + 0.05 / (double)speed_value();
        return;
    }

    // Everything useful has been handled. Re-scan later if the chest remains open.
    chest_stealer_slot = 0;
    chest_stealer_next_click = now + 0.2;
}
