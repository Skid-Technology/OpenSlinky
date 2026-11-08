// 0x8000: Java native entry points (SlinkyHooks13): wrap the JNIEnv in a Jvm handle and forward
#include "slinky/engine.h"
#include "slinky/jids.h"
extern "C" JIds *java_natives_world_cached_jni_ids;
extern "C" unsigned char java_natives_frame_client_shutting_down_natives_check;   // set by the unload sequence: every entry below is inert once it is 1
extern "C" void slinky_logf(const char *fmt, ...);
struct Rot { float yaw, pitch; };
FN(unsigned char, is_silent_rotation_active, ());
FN(Rot, get_silent_rotation, ());
FN(void *, should_render_player_name, (Jvm *jvm, void *render_player, void *other_object));
FN(void, java_set_render_manager_player_view_x, (Jvm *jvm, void *render_manager, float value));
FN(void, java_set_render_manager_player_view_y, (Jvm *jvm, void *render_manager, float value));
FN(unsigned char, handle_chat_packet, (Jvm *jvm, void *object, void *other_object));
FN(void, send_packet_with_id, (Jvm *jvm, void *object, void *other_object, void *c));
FN(void, send_packet, (Jvm *jvm, void *object, void *other_object));
// renderPlayer is a static native with the class-file descriptor (Object,Object,DDDFF)V: the JVM passes
// the three doubles and two floats in registers. The module receives them unchanged and forwards them
// to RendererLivingEntity.doRender.
FN(void, chams_render_living_entity_entry, (Jvm *jvm, void *entity, void *other_object, double x, double d0, double d1, float f0, float f1));
extern "C" void *hook_can_render_name(JNIEnv *env, jclass, void *receiver, void *b) {
    if (java_natives_frame_client_shutting_down_natives_check) return 0;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    return should_render_player_name(&jvm, receiver, b);
}
extern "C" void hook_render_player(JNIEnv *env, jclass, void *receiver, void *b, double x, double d0, double d1, float f0, float f1) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    static int logged;
    if (!logged) {
        logged = 1;
        slinky_logf("frame: first render_player");
    }
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    chams_render_living_entity_entry(&jvm, receiver, b, x, d0, d1, f0, f1);
}
// silent-rotation aware pitch / yaw setters
extern "C" void hook_set_render_x(JNIEnv *env, jclass, void *receiver, float pitch) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    if (is_silent_rotation_active()) pitch = get_silent_rotation().pitch;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    java_set_render_manager_player_view_x(&jvm, receiver, pitch);
}
extern "C" void hook_set_render_y(JNIEnv *env, jclass, void *receiver, float yaw) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    if (is_silent_rotation_active()) yaw = get_silent_rotation().yaw;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    java_set_render_manager_player_view_y(&jvm, receiver, yaw);
}
extern "C" void hook_send_packet_1_7(JNIEnv *env, jclass, void *receiver, void *b, void *c) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (!handle_chat_packet(&jvm, receiver, b)) send_packet_with_id(&jvm, receiver, b, c);
}
extern "C" void hook_send_packet_1_8(JNIEnv *env, jclass, void *receiver, void *b) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (!handle_chat_packet(&jvm, receiver, b)) send_packet(&jvm, receiver, b);
}

FN(void *, get_minecraft, (Jvm *jvm));
FN(void *, get_world, (Jvm *jvm, void *minecraft));
FN(void, set_block_meta, (Jvm *jvm, void *block, int a, int b));
struct Event { long long type; Jvm *jvm; void *world; int z; int a; int b; int zz[9]; };
extern "C" char java_natives_world_module_manager[];
FN(void, dispatch_event, (void *mgr, Event *e));
extern "C" void hook_unload_chunk(JNIEnv *env, jclass, void *receiver, int a, int b) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    void *world = get_world(&jvm, get_minecraft(&jvm));
    set_block_meta(&jvm, receiver, a, b);
    Event e = {0x14, &jvm, world, 0, a, b, {}};
    dispatch_event(java_natives_world_module_manager, &e);
}

// ---- world-change hooks: report the change to the modules (event 0x14), then call the game method ----
FN(int, get_block_id, (Jvm *jvm, void *blockOrState));
FN(void *, set_block_state, (Jvm *jvm, void *block, int a, int b, int c, void *blk, int d, int e));
struct Pos3 { int x, y, z; };
FN(Pos3, get_block_position, (Jvm *jvm, void *pos));
FN(void *, get_block_from_pos, (Jvm *jvm, void *blk));
FN(void *, set_block_at_pos, (Jvm *jvm, void *block, void *pos, void *blk, int f));
FN(void, update_chunk, (Jvm *jvm, void *chunk, int p4, int p5, int p6, int p7, int p8, int p9));
FN(int, chunk_coordinate, (int v));
struct Ev14 { long long type; Jvm *jvm; void *o; int kind; int v[4]; int z[7]; };
extern "C" void *hook_set_block_1_7(JNIEnv *env, jclass, void *receiver, int a, int b, int c, void *blk, int d, int e) {
    if (java_natives_frame_client_shutting_down_natives_check) return 0;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    int block_id = get_block_id(&jvm, blk);
    Ev14 ev = {0x14, &jvm, receiver, 2, {a, b, c, block_id}, {}};
    dispatch_event(java_natives_world_module_manager, (Event *)&ev);
    return set_block_state(&jvm, receiver, a, b, c, blk, d, e);
}
extern "C" void *hook_set_block_1_8(JNIEnv *env, jclass, void *receiver, void *pos, void *blk, int f) {
    if (java_natives_frame_client_shutting_down_natives_check) return 0;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    Pos3 block_position = get_block_position(&jvm, pos);
    int block_id = get_block_id(&jvm, get_block_from_pos(&jvm, blk));
    Ev14 ev = {0x14, &jvm, receiver, 2, {block_position.x, block_position.y, block_position.z, block_id}, {}};
    dispatch_event(java_natives_world_module_manager, (Event *)&ev);
    return set_block_at_pos(&jvm, receiver, pos, blk, f);
}
extern "C" void hook_invalidate_chunk(JNIEnv *env, jclass, void *receiver, int p4, int p5, int p6, int p7, int p8, int p9) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    update_chunk(&jvm, receiver, p4, p5, p6, p7, p8, p9);
    int cx = chunk_coordinate(p4);
    int cz = chunk_coordinate(p6);
    Ev14 ev = {0x14, &jvm, receiver, 1, {cx, cz, 0, 0}, {}};
    dispatch_event(java_natives_world_module_manager, (Event *)&ev);
}
extern "C" char *ptr_to_safe_walk_module_descriptor;
FN(void *, get_local_player, (Jvm *jvm, void *minecraft));
FN(unsigned char, get_render_flag, ());
FN(unsigned char, is_sneaking, (Jvm *jvm, void *object));
// isSneaking hook: report true for the local player while the "sneak" module asks for it
extern "C" unsigned char hook_should_clip_ledge(JNIEnv *env, jclass, void *receiver) {
    if (java_natives_frame_client_shutting_down_natives_check) return 0;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (ptr_to_safe_walk_module_descriptor[0x14] == 1) {
        void *player = get_local_player(&jvm, get_minecraft(&jvm));
        if (player && (*jvm.env)->IsSameObject(jvm.env, (jobject)player, (jobject)receiver) && get_render_flag()) return 1;
    }
    return is_sneaking(&jvm, receiver);
}

// ---- player update / velocity hooks ----
extern "C" char java_natives_frame_module_manager_target_state[];
extern "C" void *init_profile_list, *java_natives_world_key_state_table_one_byte;
extern "C" char *ptr_to_gui_module_descriptor;
struct HCtx { Jvm *jvm; void *module_manager; void *profile_list; void *minecraft; void *world; void *player; void *key_state_table; void *target;
              unsigned char in_game; unsigned char padding_41[3]; int padding_44; };
struct HEvent { int type; int padding_04; HCtx *ctx; char rest[0x38]; };
struct RotEvent { int type; int padding_04; unsigned char a, b, d, e; Rot rot; char rest[0x34]; };   // event 0xd
FN(unsigned char, is_in_game, (Jvm *jvm, void *minecraft));
FN(void, request_rotation_reset_if_flagged, (HCtx *ctx));
FN(void, update_entities, (Jvm *jvm, void *object));
FN(Rot, get_entity_rotation, (Jvm *jvm, void *entity));
FN(unsigned char, is_event_handled, (void *mgr, int type));
FN(void, apply_player_rotation, (HCtx *ctx, unsigned char a, unsigned char b, unsigned char d, Rot rotation));
FN(void, set_rotation_active, (Jvm *jvm, void *player, bool on));
extern "C" void hook_update_entities(JNIEnv *env, jclass, void *receiver) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    void *minecraft = get_minecraft(&jvm);
    void *player = get_local_player(&jvm, minecraft);
    if (!player) {
        // updateEntities also runs in the menu / after disconnect. Keep the game's world tick intact;
        // only the module/rotation part below needs a player.
        if (receiver) update_entities(&jvm, receiver);
        return;
    }
    HCtx ctx;
    ctx.jvm = &jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = receiver; ctx.player = player;
    ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
    ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(&jvm, minecraft);
    *(int *)&ctx.padding_41[0] = 0;
    ctx.padding_44 = 0;
    static int entities_logged;
    if (!entities_logged) {
        entities_logged = 1;
        slinky_logf("frame: first update_entities in_game=%d gui_open=%d", (int)ctx.in_game, (int)ptr_to_gui_module_descriptor[0x14]);
    }
    HEvent e = {};
    e.type = 0x10;
    e.ctx = &ctx;
    dispatch_event(java_natives_world_module_manager, (Event *)&e);
    request_rotation_reset_if_flagged(&ctx);
    e.type = 0x11;
    dispatch_event(java_natives_world_module_manager, (Event *)&e);
    update_entities(&jvm, ctx.world);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return;                                    // do not call more JNI with an exception pending
    }
    e.type = 0x12;
    dispatch_event(java_natives_world_module_manager, (Event *)&e);
    Rot rotation = is_silent_rotation_active() ? get_silent_rotation() : get_entity_rotation(&jvm, player);
    unsigned char a, b, d;
    if (is_event_handled(java_natives_world_module_manager, 0xd)) {
        a = 0; b = 0; d = 0;
    } else {
        RotEvent m;
        __builtin_memset(&m, 0, sizeof m);

        m.type = 0xd;
        m.rot = rotation;
        dispatch_event(java_natives_world_module_manager, (Event *)&m);
        a = m.a; b = m.b; d = m.d;
        rotation = m.rot;
    }
    apply_player_rotation(&ctx, a, b, d, rotation);
    set_rotation_active(&jvm, player, true);
}

FN(void, set_motion, (Jvm *jvm, void *object, float x, float y, float z));
FN(void *, get_game_settings, (Jvm *jvm, void *minecraft));
FN(void *, get_held_item, (Jvm *jvm, void *item));
FN(int, get_hurt_time, (Jvm *jvm, void *player));
FN(unsigned char, is_key_down, (Jvm *jvm, void *object));
struct VelEvent { int type; int padding_04; HCtx *ctx; float x, y, z; unsigned char flag; char rest[0x2b]; };
extern "C" void hook_move_flying(JNIEnv *env, jclass, void *receiver, float x, float y, float z) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (!is_event_handled(java_natives_world_module_manager, 0x15) || !is_event_handled(java_natives_world_module_manager, 0x16)) {
        void *minecraft = get_minecraft(&jvm);
        void *player = get_local_player(&jvm, minecraft);
        if ((*jvm.env)->IsSameObject(jvm.env, (jobject)receiver, (jobject)player)) {
            void *world = get_world(&jvm, minecraft);
            HCtx ctx;
            ctx.jvm = &jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = world; ctx.player = player;
            ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
            ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(&jvm, minecraft);
            *(int *)&ctx.padding_41[0] = 0;
            ctx.padding_44 = 0;
            void *held_item = get_held_item(&jvm, get_game_settings(&jvm, minecraft));
            unsigned char flag = get_hurt_time(&jvm, player) > 0 ? is_key_down(&jvm, held_item) : 0;
            VelEvent e = {0x15, 0, &ctx, x, y, z, flag, {}};
            dispatch_event(java_natives_world_module_manager, (Event *)&e);
            set_motion(&jvm, player, e.x, e.y, e.z);
            e.type = 0x16;
            dispatch_event(java_natives_world_module_manager, (Event *)&e);
            return;
        }
    }
    set_motion(&jvm, receiver, x, y, z);
}

// GUI tick: let modules ask for key pass-through, then forward movement keys into the open screen
FN(void, gui_key_passthrough, (Jvm *jvm, void *object));
FN(void *, get_current_screen, (Jvm *jvm, void *minecraft));
FN(void *, get_game_screen, (Jvm *jvm, void *object));
FN(int, get_event_key, (Jvm *jvm, void *object));
FN(unsigned char, keyboard_has_events, (Jvm *jvm));
FN(unsigned char, keyboard_has_next, (Jvm *jvm));
FN(unsigned char, keyboard_can_repeat, (Jvm *jvm));
FN(int, keyboard_next_key, (Jvm *jvm));
FN(void, keyboard_send_key, (Jvm *jvm, void *scr, int x, int y));
struct FlagEvent { int type; int padding_04; HCtx *ctx; unsigned char flag; char rest[0x37]; };
extern "C" void hook_update_controller(JNIEnv *env, jclass, void *receiver) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    gui_key_passthrough(&jvm, receiver);
    void *minecraft = get_minecraft(&jvm);
    void *player = get_local_player(&jvm, minecraft);
    void *world = get_world(&jvm, minecraft);
    HCtx ctx;
    ctx.jvm = &jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = world; ctx.player = player;
    ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
    ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(&jvm, minecraft);
    *(int *)&ctx.padding_41[0] = 0;
    ctx.padding_44 = 0;
    FlagEvent e = {};
    e.type = 0x19;
    e.ctx = &ctx;
    dispatch_event(java_natives_world_module_manager, (Event *)&e);
    if (!(e.flag & 1)) return;
    void *scr = get_current_screen(&jvm, minecraft);
    int key = get_event_key(&jvm, get_game_screen(&jvm, get_game_settings(&jvm, minecraft)));
    while (keyboard_has_events(&jvm)) {
    }
    while (keyboard_has_next(&jvm)) {
        if (!scr || !keyboard_can_repeat(&jvm)) continue;
        int k = keyboard_next_key(&jvm);
        if (k == 1 || k == key) keyboard_send_key(&jvm, scr, 0, k);
    }
}

// ---- glClear(GL_DEPTH_BUFFER_BIT) hook: fire the 3D (6) and 2D (7) render events with GL state set up ----
struct Vec3 { double x, y, z; };
FN(void *, get_render_view_entity, (Jvm *jvm, void *minecraft));
FN(void *, get_timer, (Jvm *jvm, void *minecraft));
FN(float, get_partial_ticks, (Jvm *jvm, void *timer));
FN(Vec3, java_get_entity_pos_x, (Jvm *jvm, void *entity));
FN(Vec3, java_get_entity_prev_pos_x, (Jvm *jvm, void *entity));
FN(Vec3, vec3_lerp, (Vec3 a, Vec3 b, double t));
FN(void, gl_push_state, (Jvm *jvm));
FN(void, gl_clear_color, (Jvm *jvm));
FN(void, gl_disable_lighting, (Jvm *jvm));
FN(void, gl_pop_state, (Jvm *jvm));
FN(void, gl_blend_mode, (Jvm *jvm, unsigned src, unsigned dst));
FN(void, gl_bind_texture, (Jvm *jvm));
FN(void, gl_enable_depth, (Jvm *jvm));
FN(void, gl_begin_2d, (Jvm *jvm));
FN(void, gl_depth_mask, (Jvm *jvm));
FN(void, gl_set_color, (Jvm *jvm, float r, float g, float b, float a));
FN(void, set_silent_rotation_active, (Jvm *jvm, void *player, bool on));
typedef void (*GlCap)(unsigned);
typedef void (*GlBlend)(unsigned, unsigned);
typedef void (*GlGetD)(unsigned, double *);
typedef void (*GlVoid)();
typedef void (*GlOrtho)(double, double, double, double, double, double);
typedef void (*GlColor4)(float, float, float, float);
extern "C" GlCap *ptr_to_pGlDisable, *ptr_to_pGlEnable, *ptr_to_pGlClear, *ptr_to_pGlMatrixMode;
extern "C" GlBlend *ptr_to_pGlBlendFunc;
extern "C" GlGetD *ptr_to_pGlGetFloatv;
extern "C" GlVoid *ptr_to_pGlLoadIdentity;
extern "C" GlOrtho *ptr_to_pGlOrtho;
extern "C" GlColor4 *ptr_to_pGlColor4f;
extern "C" const float one_b;
extern "C" void *frame_render_target;
extern "C" int render_target_width, render_target_height;
extern "C" double gl_modelview_matrix[16], gl_projection_matrix[16];
struct R3D { long long type; Vec3 cam; HCtx *ctx; float pt; unsigned char flag; char rest[0x1b]; };
struct R2D { long long type; Vec3 cam; HCtx *ctx; float w, h; double *proj, *mv; float pt; unsigned char flag, padding_5, padding_6, padding_7; };
extern "C" void render_world_events(JNIEnv *env, unsigned mask, unsigned char flag) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    if (!frame_render_target) return;
    static int events_logged;
    if (!events_logged) {
        events_logged = 1;
        slinky_logf("frame: first post_world_clear mask=%x flag=%d target=%p", mask, (int)flag, frame_render_target);
    }
    if (mask == 0x100) {
        Jvm jvm = {java_natives_world_cached_jni_ids, env};
        void *minecraft = get_minecraft(&jvm);
        void *player = get_local_player(&jvm, minecraft);
        unsigned char silent = is_silent_rotation_active();
        if (silent) set_silent_rotation_active(&jvm, player, true);
        unsigned char no3d = is_event_handled(java_natives_world_module_manager, 6);
        unsigned char no2d = is_event_handled(java_natives_world_module_manager, 7);
        if (!(no3d & no2d)) {
            void *world = get_world(&jvm, minecraft);
            void *view = get_render_view_entity(&jvm, minecraft);
            static int worldrender_logged;
            if (!worldrender_logged) {
                worldrender_logged = 1;
                slinky_logf("worldrender: no3d=%d no2d=%d player=%p world=%p view=%p", (int)no3d, (int)no2d, player, world, view);
            }
            if (player && world && view) {
                HCtx ctx;
                ctx.jvm = &jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = world; ctx.player = player;
                ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
                ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(&jvm, minecraft);
                *(int *)&ctx.padding_41[0] = 0;
                ctx.padding_44 = 0;
                float partial_ticks = get_partial_ticks(&jvm, get_timer(&jvm, minecraft));
                Vec3 cur = java_get_entity_pos_x(&jvm, view);
                Vec3 prev = java_get_entity_prev_pos_x(&jvm, view);
                Vec3 cam = vec3_lerp(prev, cur, (double)partial_ticks);
                if (flag) {
                    gl_push_state(&jvm); gl_clear_color(&jvm); gl_disable_lighting(&jvm); gl_pop_state(&jvm);
                    gl_blend_mode(&jvm, 0x302, 0x303);
                }
                GlCap *dis = ptr_to_pGlDisable;
                (*dis)(0xb71); (*dis)(0xde1); (*dis)(0xb50);
                GlCap *en = ptr_to_pGlEnable;
                (*en)(0xbe2);
                (*ptr_to_pGlBlendFunc)(0x302, 0x303);
                (*en)(0xb20);
                if (!no3d) {
                    R3D e = {6, cam, &ctx, partial_ticks, flag, {}};
                    dispatch_event(java_natives_world_module_manager, (Event *)&e);
                }
                if (!(no2d & 1)) {
                    GlGetD *get = ptr_to_pGlGetFloatv;
                    (*get)(0xba6, gl_modelview_matrix);
                    (*get)(0xba7, gl_projection_matrix);
                    GlCap *mode = ptr_to_pGlMatrixMode;
                    (*mode)(0x1701);
                    GlVoid *push = ptr_to_pGlLoadIdentity;
                    (*push)();
                    (*ptr_to_pGlOrtho)(0.0, (double)render_target_width, (double)render_target_height, 0.0, -3000.0, 3000.0);
                    (*mode)(0x1700);
                    (*push)();
                    R2D e = {7, cam, &ctx, (float)render_target_width, (float)render_target_height, gl_modelview_matrix, gl_projection_matrix, partial_ticks, flag, 0, 0, 0};
                    dispatch_event(java_natives_world_module_manager, (Event *)&e);
                }
                if (flag) {
                    gl_bind_texture(&jvm); gl_enable_depth(&jvm); gl_begin_2d(&jvm); gl_depth_mask(&jvm);
                    float one = one_b;
                    gl_set_color(&jvm, one, one, one, 1.0f);
                }
                (*en)(0xde1); (*en)(0xb71); (*en)(0xb44); (*en)(0xbc0);
                float one = one_b;
                (*ptr_to_pGlColor4f)(one, one, one, one);
            }
        }
        if (silent) set_silent_rotation_active(&jvm, player, false);
    }
    (*ptr_to_pGlClear)(mask);
}

// ---- the client's input handler (installed in input_input_handler_hooked_into_game for the game-window hook, see protected/input.cpp) ----
// Keeps the key-state table (byte per key: bit0 = down, bit1 = toggled by synthetic events), attaches to the JVM,
// lets the key-bind logic handle the event, and passes keys/buttons on to the registered key listeners.
// Returns true when the event was consumed.
struct InEvt { int type; int code; int padding_08; unsigned char down, synthetic; };
struct VmFns { void *fn[8]; };
extern "C" VmFns **init_javavm;                       // JavaVM*
extern "C" void *java_natives_world_key_state_table_one_byte, *java_natives_frame_profile_key_binds_key_profile, *java_natives_frame_module_key_binds_key_module;   // key states, listener maps (typed / always)
extern "C" char java_natives_world_module_manager[];                        // module manager
extern "C" char *ptr_to_gui_module_descriptor, *ptr_to_friends_module_descriptor;          // settings blocks (+0x14: flag)
FN(unsigned char, gui_handle_input_event_entry, (InEvt *e, unsigned char unchanged, void *mgr));
FN(void *, get_minecraft, (Jvm *j));
FN(void, friends_friends_friend_list, (Jvm *jvm, void *mgr));
FN(bool, key_map_dispatch, (void *map, int id, int arg));
extern "C" bool on_input_event(InEvt *e) {
    bool unchanged = true;
    if (e->type == 2) {
        unsigned char *keys = (unsigned char *)java_natives_world_key_state_table_one_byte;
        unsigned code = e->code;
        unsigned char cur = keys[code];
        unsigned char was = cur & 1;
        unsigned char down = e->down;
        if (!(e->synthetic == 1 && was == down)) {
            if (e->synthetic == 1) down = (down | (cur & 2)) ^ 2;
            keys[code] = down;
            unchanged = was == e->down;
        }
    }
    void *env;
    if (((int (*)(void *, void **, int))(*init_javavm)->fn[6])(init_javavm, &env, 0x10008)) return false;
    Jvm jvm = {java_natives_world_cached_jni_ids, (JNIEnv *)env};
    bool r = gui_handle_input_event_entry(e, unchanged, java_natives_world_module_manager);
    if (r) return r;
    if (e->type != 2 || unchanged) return r;
    if (e->code == 0) return r;
    if (!is_in_game(&jvm, get_minecraft(&jvm))) return r;
    if (ptr_to_gui_module_descriptor[0x14] != 0) return r;
    if (ptr_to_friends_module_descriptor[0x14] == 1 && e->type == 2 && e->down == 1 && e->code == 3) friends_friends_friend_list(&jvm, java_natives_world_module_manager);
    if (key_map_dispatch(java_natives_frame_profile_key_binds_key_profile, e->code, e->down)) return r;
    key_map_dispatch(java_natives_frame_module_key_binds_key_module, e->code, e->down);
    return r;
}
