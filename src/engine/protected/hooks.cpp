// Readable version of the native getMouseOver hook (0x7410) including its first-call GUI setup; notes on the
// protector-touched functions around it (0x6e10 hook control is in init.cpp, CRT 0x4cd50 is not client code).
#include "slinky/engine.h"
#include "slinky/jids.h"
#include "slinky/platform.h"

struct HCtx { Jvm *jvm; void *module_manager; void *profile_list; void *minecraft; void *world; void *player; void *key_state_table; void *target;
              unsigned char in_game; unsigned char padding_41[3]; int padding_44; };
// event 8 ("reach"): handlers may set `use` and change reach / expand; reach_distance_check reads ctx..expand as ReachA
struct ReachEvent { long long type; HCtx *ctx; unsigned char use; char padding_11[7]; double reach, expand; char rest[0x20]; };
struct PlainEvent { long long type; HCtx *ctx; char rest[0x38]; };
struct Pair16 { long long a, b; };

extern "C" JIds *java_natives_world_cached_jni_ids;
extern "C" char java_natives_world_module_manager[], java_natives_frame_module_manager_target_state[];   // module manager, target state
extern "C" void *init_profile_list, *java_natives_world_key_state_table_one_byte;
extern "C" char *ptr_to_gui_module_descriptor;                 // client settings block (+0x14: GUI open)
extern "C" Pair16 init_slinkychannelhandler_class_constructor;                 // netty handler class + ctor
extern "C" unsigned char hooks_one_time_initialisation_done;          // one-time initialisation done
extern "C" unsigned char java_natives_frame_client_shutting_down_natives_check;          // set by the unload sequence: the entry is inert once it is 1
extern "C" const double hooks_default_reach_3_0_hitbox[2];       // default reach 3.0, hitbox expand 0.1

FN(void, call_get_mouse_over, (Jvm *jvm, void *renderer, float partial_ticks));   // EntityRenderer.getMouseOver (original)
FN(void *, get_minecraft, (Jvm *jvm));                                       // Minecraft instance
FN(int, register_channel_class, (Jvm *jvm, Pair16 *handler));                          // keep "slinky_handler" in the netty pipeline
FN(void, update_mouse_sensitivity_step, (Jvm *jvm, void *minecraft));
FN(void *, get_local_player, (Jvm *jvm, void *minecraft));                             // player
FN(void *, get_world, (Jvm *jvm, void *minecraft));                             // world
FN(unsigned char, is_in_game, (Jvm *jvm, void *minecraft));                      // in game (no screen open)
FN(bool, target_clear, (HCtx *ctx, void *target));
FN(void, dispatch_event, (void *mgr, void *event));                         // dispatch an event to the modules
FN(void, reach_distance_check, (void *reach_args));                                // entity mouse-over with module reach
FN(void, release_tracked_owner, ());

// ---- GUI setup, once, on the render thread of the game (needs its OpenGL context) ----
// New code (the original does this inside its virtualised first-call block): load the GL entry points, create the
// NanoVG context the GUI draws with, and the window object (GuiWindow, created below) that carries it and the font ids.
extern "C" int slinky_load_gl(void *(*get)(const char *name));
extern "C" void *nvgCreateGL3(int flags);
enum { NVG_ANTIALIAS = 1, NVG_STENCIL_STROKES = 2 };   // inferred: the usual flags; the shader is the EDGE_AA variant
struct GuiWindow { void *ig; double t; int w, h; float sw, sh, scale; int padding_24; void *fb; float clear[4];
                   int font[2]; int tex; float tw, th; };       // +0x48 texture, +0x4c/+0x50 atlas size
extern "C" GuiWindow *frame_render_target;
extern "C" char gui_state_block[];                                   // GUI state (gui_state_block in src/data, logic/visual/gui.cpp)
extern "C" const unsigned char hooks_embedded_truetype_inter_medium_inter[], hooks_embedded_truetype_inter_medium_inter_second[];   // embedded TrueType: Inter Medium, Inter Bold
extern "C" const unsigned char hooks_64x64_gui_icon_atlas[];                   // 64x64 GUI icon atlas (PNG, 5129 bytes)
extern "C" int nvgCreateImageMem(void *ctx, int imageFlags, unsigned char *data, int ndata);
FN(void *, libc_calloc, (unsigned long long n, unsigned long long size));
FN(void, libc_free, (void *ptr));
FN(int, find_glfw_entrypoint, (void **ctx, unsigned slot, const char *name, const unsigned char *data, int size));
// The click GUI's tab bar: 7 entries {int kind; int pad; const char *label} at state+0x2e0, rendered by draw_tab_bar
// and indexed by the selected tab (state+0x350) in draw_gui_frame. The image leaves the array zeroed in .data and fills
// it during start-up (the virtualised block, not recovered) - without this the first GUI frame calls nvgText(NULL).
struct GuiTab { int kind; int padding_04; const char *label; };
static const char *const tab_label[7] = {"Combat", "Move", "Visual", "Block", "Utility", "Profiles", "Unload"};
static void gui_tabs_init() {
    GuiTab *t = (GuiTab *)(gui_state_block + 0x2e0);
    for (int i = 0; i < 7; i++) {
        t[i].kind = i;
        t[i].label = tab_label[i];
    }
}
static int gui_init() {
    gui_tabs_init();
    slinky_load_gl(slinky_gl_get_proc);
    GuiWindow *w = (GuiWindow *)libc_calloc(1, sizeof(GuiWindow));
    if (!w) return 1;                              // retryable allocation failure
    w->ig = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    w->scale = 1.0f;
    if (!w->ig) {
        slinky_logf("GUI setup failed (no NanoVG context)");
        libc_free(w);
        return 1;                                  // retry next frame
    }
    if (find_glfw_entrypoint((void **)w, 0, "Inter Medium", hooks_embedded_truetype_inter_medium_inter, 694511) ||
        find_glfw_entrypoint((void **)w, 1, "Inter Bold", hooks_embedded_truetype_inter_medium_inter_second, 715203)) {
        slinky_logf("GUI setup failed (font load)");
        libc_free(w);                              // the NanoVG context has no safe delete entry point here
        return 2;                                  // permanent failure: do not leak a new context every frame
    }
    // GUI icons: one 64x64 sheet of 20px tiles, used by draw_texture (profile load/save/folder/delete and
    // friends). The original start-up uploaded it and stored {image, 64, 64} at window+0x48/+0x4c/+0x50;
    // without it draw_texture used image 0 and every icon was a solid rectangle.
    int atlas = nvgCreateImageMem(w->ig, 0, (unsigned char *)hooks_64x64_gui_icon_atlas, 5129);
    if (atlas <= 0) {
        slinky_logf("GUI icon atlas failed to load");
    } else {
        w->tex = atlas;
        w->tw = 64.0f;
        w->th = 64.0f;
    }
    frame_render_target = w;
    return 0;
}

// One-time GUI setup. Both callers run on the client thread with the GL context current. Doing it from the
// per-frame updateDisplay hook as well matters on vanilla 1.8.9: EntityRenderer.getMouseOver only runs once a
// world is loaded, so initialising there alone left the client (and its GUI key) dormant on the main menu.
// State: 0 = idle, 1 = in progress, 2 = done, 3 = permanently failed.
extern "C" void slinky_gui_init_once() {
    unsigned char state = __atomic_load_n(&hooks_one_time_initialisation_done, __ATOMIC_SEQ_CST);
    if (state == 2 || state == 3) return;
    unsigned char expected = 0;
    if (!__atomic_compare_exchange_n(&hooks_one_time_initialisation_done, &expected, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        return;
    int result = gui_init();
    __atomic_store_n(&hooks_one_time_initialisation_done, result == 0 ? 2 : (result == 1 ? 0 : 3), __ATOMIC_SEQ_CST);
}

// ---- native hook of EntityRenderer.getMouseOver(partialTicks) ----
extern "C" void hook_get_mouse_over(JNIEnv *env, jclass, void *renderer, float partial_ticks) {
    if (java_natives_frame_client_shutting_down_natives_check) return;
    Jvm jvm = {java_natives_world_cached_jni_ids, env};
    call_get_mouse_over(&jvm, renderer, partial_ticks);   // vanilla ray trace first

    slinky_gui_init_once();   // first frame on the client thread with the GL context current (also runs in the menu)
    void *minecraft = get_minecraft(&jvm);
    Pair16 handler = init_slinkychannelhandler_class_constructor;
    if (register_channel_class(&jvm, &handler)) {   // pipeline handler could not be installed: print and clear the Java exception
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
    update_mouse_sensitivity_step(&jvm, minecraft);
    void *player = get_local_player(&jvm, minecraft);
    void *world = get_world(&jvm, minecraft);

    HCtx ctx;
    ctx.jvm = &jvm;
    ctx.module_manager = java_natives_world_module_manager;
    ctx.profile_list = init_profile_list;
    ctx.minecraft = minecraft;
    ctx.world = world;
    ctx.player = player;
    ctx.key_state_table = java_natives_world_key_state_table_one_byte;
    ctx.target = java_natives_frame_module_manager_target_state;
    ctx.in_game = ptr_to_gui_module_descriptor[0x14] ? 0 : is_in_game(&jvm, minecraft);
    ctx.padding_41[0] = ctx.padding_41[1] = ctx.padding_41[2] = 0;
    ctx.padding_44 = 0;
    target_clear(&ctx, java_natives_frame_module_manager_target_state);

    PlainEvent ev0 = {};   // event 0: mouse-over computed
    ev0.ctx = &ctx;
    dispatch_event(java_natives_world_module_manager, &ev0);

    if (!is_in_game(&jvm, minecraft)) return;
    ReachEvent ev = {};   // event 8: modules may extend the reach
    ev.type = 8;
    ev.ctx = &ctx;
    ev.reach = hooks_default_reach_3_0_hitbox[0];
    ev.expand = hooks_default_reach_3_0_hitbox[1];
    dispatch_event(java_natives_world_module_manager, &ev);
    if (ev.use == 1) reach_distance_check(&ev.ctx);
    release_tracked_owner();
}

// ---- set_java_hooks(JNIEnv *env, int enable): turn every game hook on (1) or off (0) -> init.cpp ----

// ---- crt_dll_attach / crt_dll_attach_helper: MSVC C runtime DLL attach/detach ----
// crt_dll_attach(HINSTANCE, DWORD reason, void *) matches the vcruntime DLL attach/detach dispatch: it branches on
// reason 1 / 0, keeps a process-attach counter (data_3065c4), takes the native-startup lock with `lock cmpxchg`
// on a TEB field and retries with a 1000 ms sleep, then calls into the CRT (crt_initterm). crt_dll_attach_helper is only
// called from it. This is Microsoft runtime code, not client logic (see vcruntime's dll_dllmain.cpp).

// ---- empty_placeholder(gui state): virtualised, NOT RECOVERED ----
// It prepares a message from a short string stored encrypted at rdata_140a7b (right behind "Unload Slinky?") and is
// only started through reset_key_state. The decrypted text and the effect are unknown, so this is a no-op placeholder.
extern "C" void empty_placeholder(void *gui) { (void)gui; }
