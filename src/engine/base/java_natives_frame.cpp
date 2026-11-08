// 0x7000: Java native entry points (SlinkyHooks13): wrap the JNIEnv in a Jvm handle and forward
#include "slinky/engine.h"
#include "slinky/jids.h"
extern "C" JIds *java_natives_world_cached_jni_ids;
extern "C" void slinky_logf(const char *fmt, ...);
extern "C" char *ptr_to_frame_module_settings;   // -> module settings block
FN(void, render_world_events, (JNIEnv *env, int key, bool down));
FN(void, tick_rotation, (Jvm *jvm));
FN(void, set_local_player_max_hurt_time, (Jvm *jvm));
FN(unsigned char, world_is_loaded, (Jvm *jvm));
FN(void, slinky_gui_init_once, ());               // one-time NanoVG/GUI setup (protected/hooks.cpp)
FN(void, slinky_try_pending_class_hooks, ());     // Java 9+: deferred WorldClient retransform (protected/init.cpp)
// The unload sequence sets java_natives_frame_client_shutting_down_natives_check ("client is shutting down") before slinky_teardown frees every
// global about a second later. The JVM can still dispatch any of these natives in that window (the
// retransform is not guaranteed to have unwired every patched call site: see the java_set_render_manager_player_view_y/serialize_settings
// unload crashes in hs_err pid6708/pid2260), so every entry reachable through the hooks checks the flag
// first and does nothing.
extern "C" unsigned char java_natives_frame_client_shutting_down_natives_check;
extern "C" void hook_post_world_clear_1_7(JNIEnv *env, jclass, int key) { if (!java_natives_frame_client_shutting_down_natives_check) render_world_events(env, key, false); }
extern "C" void hook_post_world_clear_1_8(JNIEnv *env, jclass, int key) { if (!java_natives_frame_client_shutting_down_natives_check) render_world_events(env, key, true); }
extern "C" void hook_is_display_active_prerender(JNIEnv *env, jclass) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    tick_rotation(&jvm);
    if (ptr_to_frame_module_settings[0x14] == 1) set_local_player_max_hurt_time(&jvm);
    world_is_loaded(&jvm);
}

struct Rot { float yaw, pitch; };
FN(unsigned char, is_silent_rotation_active, ());
FN(Rot, get_silent_rotation, ());
FN(void *, get_minecraft, (Jvm *jvm));
FN(void *, get_local_player, (Jvm *jvm, void *minecraft));
FN(void *, get_minecraft_render_global, (Jvm *jvm, void *minecraft));
FN(void, set_render_manager_boolean, (Jvm *jvm, void *render_manager, bool on));
FN(void, apply_rotation_to_player, (Jvm *jvm, void *player));
FN(void, set_silent_rotation_active, (Jvm *jvm, void *player, bool on));
FN(void, call_orient_camera, (Jvm *jvm, void *object, float t));
FN(Rot, get_entity_rotation, (Jvm *jvm, void *entity));
FN(Rot, get_entity_prev_rotation, (Jvm *jvm, void *entity));
FN(void, set_entity_rotation, (Jvm *jvm, void *entity, Rot rotation));
FN(void, set_entity_prev_rotation, (Jvm *jvm, void *entity, Rot rotation));
FN(void, set_active_render_info, (Jvm *jvm, void *active_render_info, bool b));
// camera orientation hook: with silent rotations active, orient using the server rotation
extern "C" void hook_orient_camera(JNIEnv *env, jclass, void *receiver, float t) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (is_silent_rotation_active()) {
        void *minecraft = get_minecraft(&jvm);
        void *player = get_local_player(&jvm, minecraft);
        set_render_manager_boolean(&jvm, get_minecraft_render_global(&jvm, minecraft), true);
        apply_rotation_to_player(&jvm, player);
        set_silent_rotation_active(&jvm, player, true);
        call_orient_camera(&jvm, receiver, t);
        set_silent_rotation_active(&jvm, player, false);
    } else {
        call_orient_camera(&jvm, receiver, t);
    }
}
// same for the other hook: temporarily swap in the silent rotation around the call
extern "C" void hook_update_active_render_info(JNIEnv *env, jclass, void *receiver, unsigned char b) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    if (is_silent_rotation_active()) {
        void *minecraft = get_minecraft(&jvm);
        void *player = get_local_player(&jvm, minecraft);
        Rot rotation = get_entity_rotation(&jvm, player);
        Rot entity_prev_rotation = get_entity_prev_rotation(&jvm, player);
        Rot silent_rotation = get_silent_rotation();
        set_entity_rotation(&jvm, player, silent_rotation);
        set_entity_prev_rotation(&jvm, player, silent_rotation);
        set_active_render_info(&jvm, receiver, b != 0);
        set_entity_rotation(&jvm, player, rotation);
        set_entity_prev_rotation(&jvm, player, entity_prev_rotation);
    } else {
        set_active_render_info(&jvm, receiver, b != 0);
    }
}

// ---- tick hooks: build the module Ctx, dispatch pre-event (0xe), run the game method, dispatch post (0xf) ----
extern "C" char java_natives_world_module_manager[], java_natives_frame_module_manager_target_state[];
extern "C" void *init_profile_list, *java_natives_world_key_state_table_one_byte;
extern "C" char *ptr_to_gui_module_descriptor;
struct HCtx { Jvm *jvm; void *module_manager; void *profile_list; void *minecraft; void *world; void *player; void *key_state_table; void *target;
              unsigned char in_game; unsigned char padding_41[3]; int padding_44; };
struct HEvent { int type; int padding_04; HCtx *ctx; unsigned char flag, cancel; char rest[0x36]; };
FN(void *, get_world, (Jvm *jvm, void *minecraft));
FN(unsigned char, is_in_game, (Jvm *jvm, void *minecraft));
FN(void, tick_pre_click, (HCtx *ctx, void *state));
FN(void, dispatch_event, (void *mgr, HEvent *e));
FN(void, java_call_minecraft_click_mouse, (Jvm *jvm, void *minecraft));
FN(void, java_call_minecraft_right_click_mouse, (Jvm *jvm, void *minecraft));
FN(void, tick_click, (HCtx *ctx));
FN(void, tick_click_done, (HCtx *ctx));
static inline __attribute__((always_inline)) void make_ctx(HCtx *ctx, Jvm *jvm, void *minecraft) {
    void *world = get_world(jvm, minecraft);
    void *player = get_local_player(jvm, minecraft);
    ctx->jvm = jvm;
    ctx->module_manager = java_natives_world_module_manager;
    ctx->profile_list = init_profile_list;
    ctx->minecraft = minecraft;
    ctx->world = world;
    ctx->player = player;
    ctx->key_state_table = java_natives_world_key_state_table_one_byte;
    ctx->target = java_natives_frame_module_manager_target_state;
    ctx->in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(jvm, minecraft);
    ctx->padding_41[0] = 0; ctx->padding_41[1] = 0; ctx->padding_41[2] = 0;
    *(int *)&ctx->padding_41[0] = 0;
    ctx->padding_44 = 0;
}
extern "C" void hook_click_mouse(JNIEnv *env, jclass, void *minecraft) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    HCtx ctx;
    make_ctx(&ctx, &jvm, minecraft);
    tick_pre_click(&ctx, java_natives_frame_module_manager_target_state);
    HEvent e = {};
    e.type = 0xe;
    e.ctx = &ctx;
    e.flag = 1;
    dispatch_event(java_natives_world_module_manager, &e);
    if (e.cancel) return;
    java_call_minecraft_click_mouse(&jvm, minecraft);
    e.type = 0xf;
    dispatch_event(java_natives_world_module_manager, &e);
}
extern "C" void hook_right_click_mouse(JNIEnv *env, jclass, void *minecraft) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    HCtx ctx;
    make_ctx(&ctx, &jvm, minecraft);
    HEvent e = {};
    e.type = 0xe;
    e.ctx = &ctx;

    e.cancel = 0;
    dispatch_event(java_natives_world_module_manager, &e);
    tick_click(&ctx);
    if (e.cancel) return;
    java_call_minecraft_right_click_mouse(&jvm, minecraft);
    e.type = 0xf;
    dispatch_event(java_natives_world_module_manager, &e);
    tick_click_done(&ctx);
}

// ---- main per-frame hook: context, GUI frame (events 9/10), module ticks, unload ----
FN(void, hook_game_window, ());
FN(void *, get_timer, (Jvm *jvm, void *minecraft));
FN(void, invoke_callback, (void *x));
FN(void, apply_module_state, (void *mgr, void *mod, int v));
FN(double, time_now, ());
FN(int, display_width, (Jvm *jvm));
FN(int, display_height, (Jvm *jvm));
FN(unsigned char, is_event_handled, (void *mgr, int type));
FN(unsigned char, resize_render_target, (void *gui, int w, int h));
FN(double, gui_get_scale_entry, ());
FN(void, set_render_scale, (void *gui, double t));
FN(void, begin_frame_framebuffer, (void *gui, double t));
FN(void, clear_frame, (void *gui));
FN(void, begin_frame, (void *gui, double t));
FN(void, draw_framebuffer_texture_over_screen, (void *gui));
FN(void, end_frame, (void *gui));
FN(unsigned char, has_queued_modules, (void *mgr));
FN(void, apply_module_toggles, (HCtx *ctx));
FN(void, tick_modules, (HCtx *ctx, void *timer, double now));
FN(void, tick_netty, (Jvm *jvm));
FN(unsigned char, is_unload_requested_flag, ());
FN(int, save_settings, (void *module_manager, void *profile_list));
FN(void, queue_module_toggle, (void *module_manager, void *module, unsigned char enabled));
struct Pair16 { long long a, b; };
FN(int, unregister_channel_class, (Jvm *jvm, Pair16 *p));
FN(void, tick_post_frame, (HCtx *ctx, void *state));
FN(int, set_java_hooks, (JNIEnv *env, int x));
extern "C" char gui_state_block[];            // GUI state; [0x4cb] = unload requested
FN(void, restore_game_window, ());
FN(void, start_unload_thread, (void *x));
typedef void (*GlPushAttrib)(unsigned);
typedef void (*GlPopAttrib)();
extern "C" GlPushAttrib *ptr_to_pGlPushAttrib;
extern "C" GlPopAttrib *ptr_to_pGlPopAttrib;
extern "C" const double threshold_0_00833333;
extern "C" double frame_start_time;
extern "C" unsigned char frame_prev_module_state, frame_input_flag, frame_repass_needed, frame_spinlock, java_natives_frame_client_shutting_down_natives_check;
extern "C" int frame_resize_cooldown, render_target_width, render_target_height;
extern "C" void *java_natives_frame_module_key_binds_key_module, *frame_render_target, *init_our_module_handle;
extern "C" Pair16 init_slinkychannelhandler_class_constructor;
struct ModEnt { char padding_00[0x1e8]; void (*on_shutdown)(HEvent *); };
struct Mgr { ModEnt **data; unsigned long long count; };
struct GuiEvent { int type; int padding_04; HCtx *ctx; int flags; float dt; char rest[0x30]; };
struct GuiEvent2 { long long type; HCtx *ctx; void *gui; float dt; char rest[0x2c]; };
static inline __attribute__((always_inline)) void spin_lock() {
    while (__atomic_exchange_n(&frame_spinlock, 1, __ATOMIC_SEQ_CST)) __builtin_ia32_pause();
}
extern "C" void hook_update_display(JNIEnv *env) {
    if (java_natives_frame_client_shutting_down_natives_check) return;   // the client is shutting down; this frame must not touch anything the
                               // unload thread is about to free (save_settings of a second pass crashed here)
    hook_game_window();
    slinky_gui_init_once();           // updateDisplay runs on the client thread with the GL context current, also in the menu
    slinky_try_pending_class_hooks();
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    void *minecraft = get_minecraft(&jvm);
    void *player = get_local_player(&jvm, minecraft);
    void *world = get_world(&jvm, minecraft);
    void *timer = get_timer(&jvm, minecraft);
    HCtx ctx;
    ctx.jvm = &jvm; ctx.module_manager = java_natives_world_module_manager; ctx.profile_list = init_profile_list; ctx.minecraft = minecraft; ctx.world = world; ctx.player = player;
    ctx.key_state_table = java_natives_world_key_state_table_one_byte; ctx.target = java_natives_frame_module_manager_target_state;
    char *cfg = ptr_to_gui_module_descriptor;
    ctx.in_game = cfg[0x14] ? 0 : is_in_game(&jvm, minecraft);
    *(int *)&ctx.padding_41[0] = 0;
    ctx.padding_44 = 0;
    if (!ctx.in_game && frame_prev_module_state) {
        invoke_callback(java_natives_frame_module_key_binds_key_module);
        if (!world_is_loaded(&jvm)) __builtin_memset(java_natives_world_key_state_table_one_byte, 0, 0x68);
    }
    frame_prev_module_state = ctx.in_game;
    if (*(int *)(cfg + 0x1c) == 0) apply_module_state(java_natives_world_module_manager, ptr_to_gui_module_descriptor, 0x63);
    double now = time_now();
    int W = display_width(&jvm);
    int H = display_height(&jvm);
    if (0.0 > frame_start_time) frame_start_time = now;
    if (frame_render_target && !is_event_handled(java_natives_world_module_manager, 0xa)) {
        render_target_width = W;
        render_target_height = H;
        if (resize_render_target(frame_render_target, W, H)) {
            frame_resize_cooldown = 0xa;
        } else if (frame_resize_cooldown > 0) {
            frame_resize_cooldown = frame_resize_cooldown - 1;
        }
        void *gui = frame_render_target;
        set_render_scale(gui, gui_get_scale_entry());
        double dt = now - frame_start_time;
        if (dt > threshold_0_00833333) {
            float fdt = (float)dt;
            GuiEvent e = {};
            e.type = 9;
            e.ctx = &ctx;
            e.dt = fdt;
            dispatch_event(java_natives_world_module_manager, (HEvent *)&e);
            int flags = e.flags;
            frame_input_flag = flags & 1;
            if (flags & 1) {
                if ((flags & 2) || frame_repass_needed || frame_resize_cooldown > 0) {
                    unsigned char again = (unsigned)(flags & 2) >> 1;
                    (*ptr_to_pGlPushAttrib)(0xffffffff);
                    begin_frame_framebuffer(frame_render_target, now);
                    GuiEvent2 e2 = {0xa, &ctx, frame_render_target, fdt, {}};
                    dispatch_event(java_natives_world_module_manager, (HEvent *)&e2);
                    clear_frame(frame_render_target);
                    (*ptr_to_pGlPopAttrib)();
                    frame_repass_needed = again;
                }
            }
            frame_start_time = now;
        }
        if (frame_input_flag) {
            (*ptr_to_pGlPushAttrib)(0xffffffff);
            begin_frame(frame_render_target, now);
            draw_framebuffer_texture_over_screen(frame_render_target);
            end_frame(frame_render_target);
            (*ptr_to_pGlPopAttrib)();
        } else {
            frame_repass_needed = 1;
        }
    }
    if (has_queued_modules(java_natives_world_module_manager)) {
        spin_lock();
        apply_module_toggles(&ctx);
        frame_spinlock = 0;
    }
    tick_modules(&ctx, timer, now);
    tick_netty(&jvm);
    if (!is_unload_requested_flag()) return;
    gui_state_block[0x4cb] = 0;   // consume the click immediately: one sequence per click, no repass if
                              // the user clicks again while this one is still running
    // Take the Java hooks out before touching any state. If removal fails the game will keep calling
    // these natives; the old sequence still freed the module manager from the unload thread and the
    // next frame crashed in save_settings -> serialize_settings on the freed module list. In that case keep the client
    // resident (everything stays allocated) and cancel the unload request instead. The native entries
    // check java_natives_frame_client_shutting_down_natives_check, so a partial removal followed by the real teardown can no longer fault
    // either: everything the game still reaches goes inert the moment this branch commits.
    Pair16 p = init_slinkychannelhandler_class_constructor;
    int channel_off = p.a ? unregister_channel_class(&jvm, &p) : 0;
    slinky_logf("unload: channel-off=%d", channel_off);
    // 0x3fd means there is no live network pipeline; there is nothing to remove in that case.
    if (channel_off && channel_off != 0x3fd) return;
    int hoff = set_java_hooks(env, 0);
    slinky_logf("unload: hooks-off=%d", hoff);
    if (hoff) return;
    // unload: save settings, notify and disable every module, tear down
    int saved = save_settings(java_natives_world_module_manager, init_profile_list);
    slinky_logf("unload: settings saved=%d", saved);
    HEvent e3 = {};
    e3.type = 0x1a;
    e3.ctx = &ctx;
    Mgr *mgr = (Mgr *)java_natives_world_module_manager;
    for (unsigned long long i = 0; i < mgr->count; i++)
        if (mgr->data[i]->on_shutdown) mgr->data[i]->on_shutdown(&e3);
    for (unsigned long long i = 0; i < mgr->count; i++) queue_module_toggle(java_natives_world_module_manager, mgr->data[i], 0);
    spin_lock();
    java_natives_frame_client_shutting_down_natives_check = 1;
    frame_spinlock = 0;
    apply_module_toggles(&ctx);
    tick_post_frame(&ctx, java_natives_frame_module_manager_target_state);
    restore_game_window();
    slinky_logf("unload: starting unload thread");
    start_unload_thread(init_our_module_handle);
}

// ---- set the key bind of a module: re-register it in the bind map, then tell the modules (event 0x18) ----
FN(void, key_map_remove, (void *map, int key, void *mod));   // unregister
FN(void, key_map_add, (void *map, int key, void *mod));   // register
struct BindEvent { int type; int padding_04; void *mgr; void *mod; char rest[0x30]; int tail; };
extern "C" void set_module_bind(void *mod, int key) {
    int *slot = (int *)((char *)mod + 0x1c);
    key_map_remove(java_natives_frame_module_key_binds_key_module, *slot, mod);
    *slot = key;
    key_map_add(java_natives_frame_module_key_binds_key_module, key, mod);
    BindEvent ev = {};
    ev.type = 0x18;
    ev.mgr = java_natives_world_module_manager;
    ev.mod = mod;
    dispatch_event(java_natives_world_module_manager, (HEvent *)&ev);
}

// ---- key handlers of the two key maps (java_natives_frame_profile_key_binds_key_profile: key -> profile name, java_natives_frame_module_key_binds_key_module: key -> module) ----
// Both are called from the input handler on_input_event through key_map_dispatch as handler(item, key, down).
FN(int, load_profile_name, (const char *name, void *mgr));              // load profile `name`; 0 = ok
FN(void *, find_profile, (void *profiles, const char *name));       // profile entry by name (+0x38: key bind)
FN(void *, key_map_get, (void *map, int key, void *val));          // item registered under `key`
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(unsigned long long, libc_strlen, (const char *str));
FN(void *, libc_memcpy, (void *dest, const void *src, unsigned long long n));
extern "C" void *java_natives_frame_profile_key_binds_key_profile;
extern "C" unsigned char frame_spinlock, java_natives_frame_client_shutting_down_natives_check;
extern "C" const char str_failed_to_load_profile[];   // "Failed to load profile"
struct Ev48 { int type; int padding_04; union { const char *name; struct { int err; int pad2; const char *msg; }; }; char rest[0x38]; };
extern "C" const char ptr_to_str_failed_to_set_bind[];   // prepared "Failed to set bind" event (0x48 bytes)
// profile bind pressed: load the profile; a failure unbinds it and is reported (event 0xc), success is event 0xb
extern "C" unsigned char on_profile_bind(const char *name, int key, bool down) {
    if (!down) return 0;
    int err = load_profile_name(name, java_natives_world_module_manager);
    if (!err) {
        Ev48 e = {};
        e.type = 0xb;
        e.name = name;
        dispatch_event(java_natives_world_module_manager, (HEvent *)&e);
        return 1;
    }
    char *prof = (char *)find_profile(init_profile_list, name);
    if (prof) *(int *)(prof + 0x38) = 0;
    Ev48 e = {};
    e.type = 0xc;
    e.err = err;
    e.msg = str_failed_to_load_profile;
    dispatch_event(java_natives_world_module_manager, (HEvent *)&e);
    key_map_remove(java_natives_frame_profile_key_binds_key_profile, key, 0);
    return 0;
}

// module bind pressed (hold mode: enabled while the key is down; otherwise toggles on press)
extern "C" unsigned char on_module_bind(char *mod, int key, bool down) {
    if (mod[0x20] == 1) {
        queue_module_toggle(java_natives_world_module_manager, mod, down);
        mod[0x21] = down;
    } else if (down) {
        queue_module_toggle(java_natives_world_module_manager, mod, mod[0x14] ^ 1);
    }
    return 1;
}

// bind profile `p` to `key` (0 = unbind): a key serves one profile, the previous owner loses its bind
extern "C" void bind_profile_to_key(char *p, int key) {
    void *map = java_natives_frame_profile_key_binds_key_profile;

    key_map_remove(map, *(int *)(p + 0x38), 0);
    void *other = key_map_get(java_natives_frame_profile_key_binds_key_profile, key, 0);
    if (other) {
        char *o = (char *)find_profile(init_profile_list, (const char *)other);
        if (o) {
            *(int *)(o + 0x38) = 0;
            key_map_remove(java_natives_frame_profile_key_binds_key_profile, key, 0);
        }
    }
    if (key) {
        const char *name = *(const char **)p;
        unsigned long long n = libc_strlen(name);
        char *copy = (char *)libc_calloc(n + 1, 1);
        if (!copy) {
            Ev48 e;
            __builtin_memcpy(&e, ptr_to_str_failed_to_set_bind, 0x48);
            dispatch_event(java_natives_world_module_manager, (HEvent *)&e);
            return;
        }
        libc_memcpy(copy, name, n);
        key_map_add(java_natives_frame_profile_key_binds_key_profile, key, copy);
    }
    *(int *)(p + 0x38) = key;
}

// ---- SlinkyChannelHandler.onPacket(Object packet, boolean outbound, long time, int id): int ----
// Netty thread -> client: serialised with the frame code through the lock at frame_spinlock, ignored once the
// client is shutting down (java_natives_frame_client_shutting_down_natives_check). The modules see it as event 2 (inbound) / 3 (outbound) and may change
// `result` (checked at +0x24 by the handler).
struct PktEv { int type; int padding_04; Jvm *jvm; void *pkt; long long time; int id; int result; char rest[0x20]; int tail; };
extern "C" int on_packet(JNIEnv *env, jobject, void *pkt, unsigned char outbound, long long time, int id) {
    bool busy;
    for (;;) {
        busy = __atomic_exchange_n(&frame_spinlock, 1, __ATOMIC_SEQ_CST) != 0;

        if (!busy) break;
        __builtin_ia32_pause();
    }
    int result = 0;
    if (!java_natives_frame_client_shutting_down_natives_check) {
        Jvm jvm = {java_natives_world_cached_jni_ids, env};
        PktEv e = {};
        e.type = outbound ? 3 : 2;
        e.jvm = &jvm;
        e.pkt = pkt;
        e.time = time;
        e.id = id;
        dispatch_event(java_natives_world_module_manager, (HEvent *)&e);
        result = e.result;
    }
    frame_spinlock = 0;
    return result;
}
