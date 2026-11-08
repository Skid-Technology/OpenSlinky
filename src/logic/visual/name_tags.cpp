// Name Tags - bigger, clearer name tags above players.
//
// Every tick the module builds a label for each player within "range" (at most 100; bots
// skipped): health (coloured red / yellow / green, optionally in hearts, plus absorption in
// gold), the name (red for others, green for friends/teammates, or the server's display name),
// and the distance. With "show_equipment" the armour and held item are drawn above it, and with
// "enchantments" up to 4 enchantments per item ("Pr 4", "Sh 5", ...).
// Every frame the labels are drawn on screen at the players' heads, through walls.
//
// Callbacks:
//   name_tags_disable  switched off: free the labels and the item renderer
//   name_tags_tick     every tick: build the labels
//   name_tags_render   every frame: draw them
#include "slinky/engine.h"
#include "slinky/jids.h"

struct Vec3 { double x, y, z; };
struct Pair { float x, y; };
struct Slot { unsigned char has; char padding_01[3]; int count; void **refs; };   // armour piece + enchant labels
struct Tag {                        // 0xa0 bytes at 0x2fc920
    Vec3 prev, pos;
    void *ent;                      // global ref
    void *name;                     // global ref to the label
    int width;
    float prev_dist, dist;
    int armor;                      // pieces worn
    Slot slot[5];
};
struct Builder { alignas(8) char raw[0x20]; };
struct Enchant { unsigned short id, lvl; };
struct RenderEvt { char padding_00[8]; Vec3 cam; Ctx *ctx; char padding_28[0x18]; float partial; unsigned char shader; };

struct EvHook { char padding_00[8]; Ctx *ctx; void *ev; unsigned char on; };

extern "C" char name_tags_module_descriptor[];           // Name Tags module descriptor
extern "C" void *name_tags_cached_ref;

FN(void, entity_list_for_render, (void *jvm));

extern "C" unsigned long long name_tags_count;     // count
extern "C" Tag name_tags_table[100];
extern "C" char name_tags_label_buffer[0x80];             // label buffer
extern "C" Enchant name_tags_enchant_table[25];
extern "C" char name_tags_enchant_label_buffer[6];                // enchant label buffer
extern "C" float name_tags_scale;                  // scale
extern "C" unsigned char name_tags_distance_scaling;          // distance scaling
extern "C" unsigned char name_tags_background;          // background
extern "C" unsigned char name_tags_text_shadow;          // text shadow
extern "C" unsigned char name_tags_show_equipment;          // armour
extern "C" unsigned char name_tags_enchantments;          // enchantments
extern "C" float name_tags_range;                  // range
extern "C" unsigned char name_tags_own_name_tag;          // own name tag
extern "C" unsigned char name_tags_display_name;          // display name
extern "C" unsigned char name_tags_show_health;          // health
extern "C" unsigned char name_tags_distance;          // distance
extern "C" int name_tags_health_format_1_hearts;                    // health format: 1 = hearts
extern "C" const char *const ptr_to_str_pr[63]; // enchantment abbreviations
S(str_id_in_name_tags);   // "id"
S(str_lvl_in_name_tags);   // "lvl"
S(str_sect_c_f);   // "\xa7%c%.*f"
S(str_sect_6_f);   // "\xa76+%.*f"
S(str_sect_r);   // heart, "\xa7r "
S(str_sect);   // "\xa7"
S(str_sect_r_sect_c_0fm);   // "\xa7r \xa7%c%.0fm"
S(fmt_str_int);   // "%s %d"
extern "C" const float clamp_bound_999, clamp_bound_99_in_name_tags, name_tag_high_health_threshold, name_tag_low_health_threshold, half_in_name_tags_a, twenty_in_name_tags, ten_in_name_tags_a;
extern "C" const float name_tag_item_z_level, half_turn_degrees_in_name_tags, factor_100_in_name_tags, factor_10_in_name_tags, one_in_name_tags, half_in_name_tags, name_tag_enchant_text_y,
                       offset_2_in_name_tags_b, name_tag_depth_epsilon;
extern "C" const double name_tag_height_offset, factor_0_5_in_name_tags, offset_neg_2_in_name_tags, offset_2_in_name_tags;
extern "C" void (**ptr_to_pGlPushMatrix)();                                 // glPushMatrix
extern "C" void (**ptr_to_pGlPopMatrix)();                                 // glPopMatrix
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);              // glTranslatef
extern "C" void (**ptr_to_pGlScalef)(float, float, float);              // glScalef
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);              // glColor3f
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);       // glColor4f
extern "C" void (**ptr_to_pGlEnable)(unsigned);                         // glEnable
extern "C" void (**ptr_to_pGlDisable)(unsigned);                         // glDisable
extern "C" void (**ptr_to_pGlBlendFunc)(unsigned, unsigned);               // glBlendFunc
extern "C" void (**ptr_to_pGlBegin)(unsigned);                         // glBegin
extern "C" void (**ptr_to_pGlVertex2f)(float, float);                     // glVertex2f
extern "C" void (**ptr_to_pGlEnd)();                                 // glEnd

FN(void *, java_get_minecraft_render_item, (void *jvm, void *minecraft));
FN(void *, java_new_render_item, (void *jvm));
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (void *jvm, void *entity));
FN(void *, java_get_world_player_entities, (void *jvm, void *world));
FN(void *, java_get_minecraft_render_manager, (void *jvm, void *minecraft));
FN(void *, java_get_render_manager_text_renderer, (void *jvm, void *minecraft));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void *, get_game_settings, (void *jvm, void *object));
FN(int, java_get_game_settings_third_person_view, (void *jvm, void *game_settings));
FN(int, entity_flags_get, (void *jvm, void *entity));
FN(unsigned char, java_get_entity_is_dead, (void *jvm, void *entity));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(double, vec3_distance, (Vec3 a, Vec3 b));
FN(void *, java_call_entity_get_formatted_command_sender_name, (void *jvm, void *entity));
FN(void *, java_call_ichat_component_get_formatted_text, (void *jvm, void *object));
FN(void *, java_call_entity_get_command_sender_name, (void *jvm, void *entity));
FN(void, buffer_init, (Builder *b, char *buf, int cap));
FN(unsigned char, buffer_append, (Builder *b, const char *s));
FN(unsigned char, buffer_printf, (Builder *b, const char *fmt, ...));
FN(unsigned char, buffer_push_char, (Builder *b, char ch));
FN(unsigned char, buffer_terminate, (Builder *b));
FN(float, java_call_entity_living_base_get_health, (void *jvm, void *entity_living_base));
FN(float, java_call_entity_player_get_absorption_amount, (void *jvm, void *entity_player));
FN(float, limit_rotation_delta, (float lo, float hi, float v));
FN(double, libc_sqrt, (double x));
FN(float, libc_sqrtf, (float x));
FN(unsigned char, java_variant, (void *jvm));
FN(Vec3, vec3_add_eye_height_sneak, (Vec3 v));
FN(int, java_call_font_renderer_get_string_width, (void *jvm, void *font, void *str));
FN(void *, java_call_entity_player_get_equipment_in_slot, (void *jvm, void *entity_player, int slot));
FN(void *, java_call_item_stack_get_enchantment_tag_list, (void *jvm, void *stack));
FN(int, java_call_nbttag_list_tag_count, (void *jvm, void *list));
FN(void *, java_call_nbttag_list_get_compound_tag_at, (void *jvm, void *list, int i));
FN(short, java_jni_call_short_method_nbttag_compound_get_short, (void *jvm, void *tag, void *key));
FN(void, libc_qsort, (void *base, unsigned long long n, unsigned long long size, int (*cmp)(const void *, const void *)));
FN(int, compare_key_table, (const void *left, const void *right));
FN(int, compare_module_height, (const void *left, const void *right));
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(void *, java_get_minecraft_render_engine, (void *jvm, void *minecraft));
FN(float, java_get_render_item_z_level, (void *jvm, void *ri));
FN(void, java_set_render_item_z_level, (void *jvm, void *ri, float z));
FN(void *, java_get_minecraft, (void *jvm, void *minecraft));
FN(float, java_call_entity_renderer_get_fovmodifier, (void *jvm, void *er, float partial));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(unsigned char, project_point_ex, (void *view, Vec3 rel, Pair *screen, float *depth));
FN(void, gl_push_state, (void *jvm));
FN(void, gl_enable_depth, (void *jvm));
FN(void, gl_pop_state, (void *jvm));
FN(void, gl_bind_texture, (void *jvm));
FN(void, gl_clear_color, (void *jvm));
FN(void, java_call_static_gl_state_manager_disable_alpha, (void *jvm));
FN(void, gl_blend_mode, (void *jvm, unsigned src, unsigned dst));
FN(void, gl_set_color, (void *jvm, float r, float g, float b, float al));
FN(void, java_font_draw_string, (void *jvm, void *font, void *str, float x, float y, unsigned color, unsigned char shadow));
FN(void, java_call_static_render_helper_enable_standard_item_lighting, (void *jvm));
FN(void, java_call_static_render_helper_disable_standard_item_lighting, (void *jvm));
FN(void, java_render_item_init, (void *jvm, void *ri, void *font, void *ir, void *stack, int x, int y));
FN(void, java_render_item_render_item_overlay_into_gui, (void *jvm, void *ri, void *font, void *ir, void *stack, int x, int y, const char *text));

typedef unsigned char (*IsSameObject)(void *env, void *a, void *b);
typedef void *(*NewGlobalRef)(void *env, void *o);


// Colour code for a value: red under `low`, yellow under `mid`, else green.
static char tier(float v, float low, float mid) {
    if (low > v)
        return 'c';
    return mid > v ? 'e' : 'a';
}

extern "C" void name_tags_disable(EvHook *hook) {
    if (!hook->on && hook->ev == name_tags_module_descriptor) {
        entity_list_for_render(hook->ctx->jvm);
        void *ref = name_tags_cached_ref;
        if (ref) {
            JNIEnv *env = ((Jvm *)hook->ctx->jvm)->env;
            (*env)->DeleteGlobalRef(env, ref);
            name_tags_cached_ref = nullptr;
        }
    }
}

extern "C" void name_tags_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    entity_list_for_render(ctx->jvm);
    if (!ctx->world)
        return;
    // An item renderer to draw the equipment with (the game's, or a new one).
    if (!name_tags_cached_ref) {
        JNIEnv *env = ((Jvm *)ctx->jvm)->env;
        void *ri;
        if (**(int **)ctx->jvm) {
            ri = java_get_minecraft_render_item(ctx->jvm, ctx->minecraft);
        } else {
            ri = java_new_render_item(ctx->jvm);
            if (!ri)
                goto have_ri;
        }
        name_tags_cached_ref = (*env)->NewGlobalRef(env, ri);
    }
have_ri:
    Vec3 position = java_get_entity_pos_x(ctx->jvm, ctx->player);
    Vec3 me_prev = java_get_entity_prev_pos_x(ctx->jvm, ctx->player);
    void *list = java_get_world_player_entities(ctx->jvm, ctx->world);
    JNIEnv *env = ((Jvm *)ctx->jvm)->env;
    void *font = java_get_render_manager_text_renderer(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    if (!font)
        return;
    void *key_id = (*env)->NewStringUTF(env, str_id_in_name_tags);
    void *key_lvl = (*env)->NewStringUTF(env, str_lvl_in_name_tags);
    if (!key_id || !key_lvl)
        return;
    for (int i = 0; i < java_call_list_size(ctx->jvm, list); i++) {
        void *element = java_call_list_get(ctx->jvm, list, i);
        unsigned char self = (*env)->IsSameObject(env, ctx->player, element);
        if (self && (name_tags_own_name_tag != 1 || !java_get_game_settings_third_person_view(ctx->jvm, get_game_settings(ctx->jvm, ctx->minecraft))))
            continue;                           // own tag only in third person
        int flags = entity_flags_get(ctx->jvm, element);
        if (flags & 2)
            continue;
        if (java_get_entity_is_dead(ctx->jvm, element))
            continue;
        Vec3 pos = java_get_entity_pos_x(ctx->jvm, element);
        double d2 = vec3_distance_sq(pos, position);
        if (d2 > (double)(name_tags_range * name_tags_range))
            continue;
        // The label text: [health] name [distance].
        JNIEnv *env2 = ((Jvm *)ctx->jvm)->env;
        void *label;
        void *name;
        if (name_tags_display_name == 1) {
            name = java_call_ichat_component_get_formatted_text(ctx->jvm, java_call_entity_get_formatted_command_sender_name(ctx->jvm, element));
            if (!name)
                continue;
            label = name;
            if (!name_tags_show_health && !name_tags_distance)
                goto have_label;
        } else {
            name = java_call_entity_get_command_sender_name(ctx->jvm, element);
            if (!name)
                continue;
        }
        {
            label = name;
            int len = (*env2)->GetStringUTFLength(env2, name);
            int n = len < 0x40 ? len : 0x40;
            const char *chars = (*env2)->GetStringUTFChars(env2, name, nullptr);
            if (!chars)
                goto have_label;
            Builder sb;
            buffer_init(&sb, name_tags_label_buffer, 0x80);
            bool ok = true;
            if (name_tags_show_health == 1) {
                float hp = limit_rotation_delta(0.0f, clamp_bound_999, java_call_entity_living_base_get_health(ctx->jvm, element));
                float abs = limit_rotation_delta(0.0f, clamp_bound_99_in_name_tags, java_call_entity_player_get_absorption_amount(ctx->jvm, element));
                char col = tier(hp, name_tag_low_health_threshold, name_tag_high_health_threshold);
                int hearts = name_tags_health_format_1_hearts == 1;
                if (hearts) {
                    hp *= half_in_name_tags_a;
                    abs *= half_in_name_tags_a;
                }
                if (!buffer_printf(&sb, str_sect_c_f, col, hearts, (double)hp))
                    ok = false;
                if (ok && abs > 0.0f && !buffer_printf(&sb, str_sect_6_f, hearts, (double)abs))
                    ok = false;
                if (ok && !buffer_append(&sb, str_sect_r))
                    ok = false;
            }
            if (ok && name_tags_display_name == 1) {
                if (!buffer_append(&sb, chars))
                    ok = false;
            } else if (ok) {
                if (!buffer_append(&sb, str_sect))
                    ok = false;
                if (ok && !buffer_push_char(&sb, (flags & 5) == 0 ? 'c' : 'a'))
                    ok = false;
                for (int k = 0; ok && len > 0 && k < n;) {
                    if ((unsigned char)chars[k] == 0xc2) {   // drop colour codes
                        k += 3;
                        continue;
                    }
                    if (!buffer_push_char(&sb, chars[k]))
                        ok = false;
                    k += 1;
                }
            }
            if (ok && name_tags_distance == 1 && !(*env)->IsSameObject(env, ctx->player, element)) {
                double d;
                if (d2 >= 0.0)
                    d = __builtin_sqrt(d2);
                else
                    d = libc_sqrt(d2);
                if (!buffer_printf(&sb, str_sect_r_sect_c_0fm, tier((float)d, ten_in_name_tags_a, twenty_in_name_tags), d))
                    ok = false;
            }
            (*env2)->ReleaseStringUTFChars(env2, name, chars);
            if (ok && buffer_terminate(&sb)) {
                void *built = (*env2)->NewStringUTF(env2, name_tags_label_buffer);
                if (built)
                    label = built;
            }
        }
    have_label:
        if (!label)
            break;
        {
            void *name_ref = (*env2)->NewGlobalRef(env2, label);
            (*env2)->DeleteLocalRef(env2, (jobject)label);
            if (!name_ref)
                break;
            void *ent_ref = (*env)->NewGlobalRef(env, element);
            if (!ent_ref) {
                (*env)->DeleteGlobalRef(env, name_ref);
                break;
            }
            // Store the tag.
            Vec3 prev = java_get_entity_prev_pos_x(ctx->jvm, element);
            if (self && java_variant(ctx->jvm)) {
                pos = vec3_add_eye_height_sneak(pos);
                prev = vec3_add_eye_height_sneak(prev);
            }
            Tag *t = &name_tags_table[name_tags_count++];
            t->pos = pos;
            t->prev = prev;
            t->ent = ent_ref;
            float f2 = (float)d2;

            float dist;
            if (f2 >= 0.0f)
                dist = __builtin_sqrtf(f2);
            else
                dist = libc_sqrtf(f2);
            t->dist = dist;
            t->prev_dist = (float)vec3_distance(me_prev, t->prev);
            t->name = name_ref;
            t->width = java_call_font_renderer_get_string_width(ctx->jvm, font, name_ref);
            t->armor = 0;
            for (int k = 0; k < 5; k++) {
                unsigned char has = java_call_entity_player_get_equipment_in_slot(ctx->jvm, element, k) != nullptr;
                if (has)
                    t->armor += 1;
                t->slot[k].has = has;
            }
            // Enchantment labels for each piece (the 4 best, by the sort order).
            if (name_tags_enchantments == 1) {
                JNIEnv *env3 = ((Jvm *)ctx->jvm)->env;
                for (int k = 0; k < 5; k++) {
                    Slot *sl = &t->slot[k];
                    sl->refs = nullptr;
                    sl->count = 0;
                    void *stack = java_call_entity_player_get_equipment_in_slot(ctx->jvm, element, k);
                    if (!stack)
                        continue;
                    void *tags = java_call_item_stack_get_enchantment_tag_list(ctx->jvm, stack);
                    if (!tags)
                        continue;
                    int nbttag_list_tag_count = java_call_nbttag_list_tag_count(ctx->jvm, tags);
                    int cnt = nbttag_list_tag_count < 0x19 ? nbttag_list_tag_count : 0x19;
                    int filled = 0;
                    for (int j = 0; nbttag_list_tag_count > 0 && j < cnt; j++) {
                        void *tag = java_call_nbttag_list_get_compound_tag_at(ctx->jvm, tags, j);
                        unsigned short nbttag_compound_get_short = java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, key_id);
                        if (nbttag_compound_get_short > 0x3e || (0x1ff0fff0ffc0fe00ull >> nbttag_compound_get_short & 1)) {
                            (*env3)->DeleteLocalRef(env3, tag);
                            continue;           // unknown enchantment
                        }
                        short lvl = java_jni_call_short_method_nbttag_compound_get_short(ctx->jvm, tag, key_lvl);
                        (*env3)->DeleteLocalRef(env3, tag);
                        lvl = lvl > 0 ? lvl : 0;
                        name_tags_enchant_table[filled].id = nbttag_compound_get_short;
                        name_tags_enchant_table[filled].lvl = lvl < 0x63 ? lvl : 0x63;
                        filled++;
                    }
                    libc_qsort(name_tags_enchant_table, (long long)filled, 4, compare_key_table);
                    int shown = filled < 4 ? filled : 4;
                    if (!shown)
                        continue;
                    sl->refs = (void **)libc_calloc((long long)shown, 8);
                    if (!sl->refs)
                        goto next;
                    for (int j = 0; j < shown; j++) {
                        unsigned id = name_tags_enchant_table[j].id;
                        const char *abbr = id <= 0x3e ? ptr_to_str_pr[id] : nullptr;
                        if (!abbr)
                            continue;
                        log_fmt(name_tags_enchant_label_buffer, 6, fmt_str_int, abbr, (int)(short)name_tags_enchant_table[j].lvl);
                        void *str = (*env3)->NewStringUTF(env3, name_tags_enchant_label_buffer);
                        if (!str)
                            goto next;
                        void *ref = (*env3)->NewGlobalRef(env3, str);
                        (*env3)->DeleteLocalRef(env3, str);
                        if (!ref)
                            goto next;
                        sl->refs[sl->count] = ref;
                        sl->count += 1;
                    }
                }
            }
        }
    next:
        if (name_tags_count == 100)
            break;
    }
    libc_qsort(name_tags_table, name_tags_count, sizeof(Tag), compare_module_height);
}

extern "C" void name_tags_render(RenderEvt *h) {
    if (!name_tags_count || !name_tags_cached_ref)
        return;
    Ctx *ctx = h->ctx;
    void *font = java_get_render_manager_text_renderer(ctx->jvm, java_get_minecraft_render_manager(ctx->jvm, ctx->minecraft));
    void *minecraft = java_get_minecraft_render_engine(ctx->jvm, ctx->minecraft);
    float saved_z = java_get_render_item_z_level(ctx->jvm, name_tags_cached_ref);
    java_set_render_item_z_level(ctx->jvm, name_tags_cached_ref, name_tag_item_z_level);
    float k = half_turn_degrees_in_name_tags / java_call_entity_renderer_get_fovmodifier(ctx->jvm, java_get_minecraft(ctx->jvm, ctx->minecraft), h->partial);
    const float one = one_in_name_tags;
    for (unsigned long long i = 0; i < name_tags_count; i++) {
        // Above the head, projected onto the screen (skipped when behind the camera).
        Tag *t = &name_tags_table[i];
        Vec3 p = vec3_lerp(t->prev, t->pos, (double)h->partial);
        p.y += name_tag_height_offset;
        Pair s;
        float depth;
        if (!project_point_ex(&h->cam, vec3_sub(p, h->cam), &s, &depth))
            continue;
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)(s.x, s.y, depth * factor_100_in_name_tags);
        float sc = depth * factor_10_in_name_tags;
        if (name_tags_distance_scaling)
            sc = sc > one ? sc : one;
        float scale = name_tags_scale * k * sc;
        (*ptr_to_pGlScalef)(scale, scale, scale);
        double x0d = (double)(-t->width) * factor_0_5_in_name_tags + offset_neg_2_in_name_tags;
        if (h->shader == 1)
            gl_push_state(ctx->jvm);
        else
            (*ptr_to_pGlDisable)(0xb71);             // GL_DEPTH_TEST
        float x0 = (float)x0d;
        if (name_tags_background == 1) {
            if (h->shader == 1) {
                gl_clear_color(ctx->jvm);
                gl_set_color(ctx->jvm, 0.0f, 0.0f, 0.0f, 0.5f);
            } else {
                (*ptr_to_pGlDisable)(0xde1);
                (*ptr_to_pGlColor4f)(0.0f, 0.0f, 0.0f, half_in_name_tags);
            }
            float x1 = (float)((double)t->width * factor_0_5_in_name_tags + offset_2_in_name_tags);
            (*ptr_to_pGlBegin)(5);                 // GL_TRIANGLE_STRIP
            (*ptr_to_pGlVertex2f)(x0, 0.0f);
            (*ptr_to_pGlVertex2f)(x1, 0.0f);
            (*ptr_to_pGlVertex2f)(x0, name_tag_enchant_text_y);
            (*ptr_to_pGlVertex2f)(x1, name_tag_enchant_text_y);
            (*ptr_to_pGlEnd)();
        }
        if (h->shader == 1) {
            gl_bind_texture(ctx->jvm);
            gl_set_color(ctx->jvm, one, one, one, 1.0f);
        } else {
            (*ptr_to_pGlEnable)(0xde1);
            (*ptr_to_pGlColor3f)(one, one, one);
        }
        java_font_draw_string(ctx->jvm, font, t->name, x0 + offset_2_in_name_tags_b, -10.0f, 0xffffffffu, name_tags_text_shadow);
        // Armour and held item above the label.
        if (name_tags_show_equipment == 1) {
            if (h->shader == 1)
                gl_enable_depth(ctx->jvm);
            else
                (*ptr_to_pGlEnable)(0xb71);
            java_call_static_render_helper_enable_standard_item_lighting(ctx->jvm);
            (*ptr_to_pGlScalef)(one, one, name_tag_depth_epsilon);
            int x = -(t->armor * 8);
            for (int slot = 0; slot < 5; slot++) {
                if (t->slot[slot].has != 1)
                    continue;
                void *stack = java_call_entity_player_get_equipment_in_slot(ctx->jvm, t->ent, slot);
                if (!stack)
                    continue;
                java_render_item_init(ctx->jvm, name_tags_cached_ref, font, minecraft, stack, x, -32);
                java_render_item_render_item_overlay_into_gui(ctx->jvm, name_tags_cached_ref, font, minecraft, stack, x, -32, nullptr);
                x += 16;
            }
            java_call_static_render_helper_disable_standard_item_lighting(ctx->jvm);
            if (h->shader) {
                gl_pop_state(ctx->jvm);
                java_call_static_gl_state_manager_disable_alpha(ctx->jvm);
                gl_blend_mode(ctx->jvm, 0x302, 0x303);
            } else {
                (*ptr_to_pGlEnable)(0xbe2);
                (*ptr_to_pGlDisable)(0xbc0);
                (*ptr_to_pGlBlendFunc)(0x302, 0x303);
            }
            (*ptr_to_pGlColor3f)(one, one, one);
            (*ptr_to_pGlScalef)(half_in_name_tags, half_in_name_tags, half_in_name_tags);
            if (name_tags_enchantments == 1) {
                int bx = -(t->armor * 8);
                for (int k2 = 0; k2 < 5; k2++) {
                    Slot *sl = &t->slot[k2];
                    if (sl->has != 1)
                        continue;
                    if (sl->refs && sl->count > 0) {
                        float lx = (float)(bx + bx);
                        int y = -64;
                        for (int j = 0; j < sl->count; j++, y += 8)
                            java_font_draw_string(ctx->jvm, font, sl->refs[j], lx, (float)y, 0xffffffffu, 1);
                    }
                    bx += 16;
                }
            }
        }
        (*ptr_to_pGlPopMatrix)();
    }
    if (h->shader) {
        gl_clear_color(ctx->jvm);
        gl_push_state(ctx->jvm);
    } else {
        (*ptr_to_pGlDisable)(0xde1);
        (*ptr_to_pGlDisable)(0xb71);
    }
    java_set_render_item_z_level(ctx->jvm, name_tags_cached_ref, saved_z);
}
