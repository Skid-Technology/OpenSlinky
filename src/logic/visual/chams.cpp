// Chams - see players through walls.
//
// Replaces the game's "render a living entity" call. For players (unless the filter says
// otherwise) it turns on a huge OpenGL polygon offset while drawing them, which pulls them in
// front of everything else, so they show through blocks.
// Friends / teammates / bots are drawn normally (with "hide_friendlies", only bots).
//
// Function:
//   chams_render_living_entity  called instead of RendererLivingEntity.doRender
#include "slinky/engine.h"

// ---- settings ----
extern "C" unsigned char chams_module_enabled;
extern "C" unsigned char chams_hide_friendlies_value;   // "hide_friendlies"

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlEnable)(unsigned);
extern "C" void (**ptr_to_pGlDisable)(unsigned);
extern "C" void (**ptr_to_pGlPolygonOffset)(float, float);
enum { GL_POLYGON_OFFSET_FILL = 0x8037 };

// ---- game / engine functions ----
FN(unsigned char, entity_flags_has, (void *jvm, void *entity, int mode));   // flags: 1/4 friend or teammate, 2 bot
FN(void, java_jni_call_nonvirtual_renderer_living_entity_do_render, (void *jvm, void *object, void *other_object, double x, double d0, double d1, float f0, float f1));   // the original render
extern "C" void slinky_logf(const char *fmt, ...);

extern "C" void chams_render_living_entity(void *jvm, void *renderer, void *entity, double x, double y, double z, float yaw, float partial) {
    static int logged;
    if (!logged) {
        logged = 1;
        slinky_logf("chams: first render wrapper enabled=%d hide_friendlies=%d", (int)chams_module_enabled, (int)chams_hide_friendlies_value);
    }

    // Off, or an entity we leave alone (bots; with "hide_friendlies" also friends/teammates: flags 7).
    if (chams_module_enabled != 1 || entity_flags_has(jvm, entity, chams_hide_friendlies_value * 5 + 2)) {
        java_jni_call_nonvirtual_renderer_living_entity_do_render(jvm, renderer, entity, x, y, z, yaw, partial);
        return;
    }

    // Draw it in front of everything.
    (*ptr_to_pGlEnable)(GL_POLYGON_OFFSET_FILL);
    (*ptr_to_pGlPolygonOffset)(1.0f, -3000000.0f);
    java_jni_call_nonvirtual_renderer_living_entity_do_render(jvm, renderer, entity, x, y, z, yaw, partial);
    (*ptr_to_pGlDisable)(GL_POLYGON_OFFSET_FILL);
}
