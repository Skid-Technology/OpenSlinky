// NanoVG entry points the client calls by their address in the original image (fn_<rva>), bound to upstream NanoVG
// (third_party/nanovg, the same version: NVGstate is 0x110 bytes and nstates sits at +0x2288 in both).
// Identified from the argument shapes and checked on the state offsets each setter writes (e.g. nvg_StrokeWidth writes
// state+0xac = strokeWidth, nvg_TextAlign state+0x108 = textAlign, nvg_Scissor clamps w/h like nvgScissor).
#include "slinky/gl_slots.h"
extern "C" {   // the GL backend headers have no C++ guard
#include "nanovg.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
}

extern "C" {
void nvg_Save(NVGcontext *nvg) { nvgSave(nvg); }
void nvg_Restore(NVGcontext *nvg) { nvgRestore(nvg); }
void nvg_BeginFrame(NVGcontext *nvg, float w, float h, float dpr) { nvgBeginFrame(nvg, w, h, dpr); }
void nvg_EndFrame(NVGcontext *nvg) { nvgEndFrame(nvg); }
NVGcolor nvg_rgb_f(float r, float g, float b) { return nvgRGBf(r, g, b); }
NVGcolor nvg_rgba_f(float r, float g, float b, float a) { return nvgRGBAf(r, g, b, a); }
NVGcolor nvg_trans_rgba_f(NVGcolor c, float a) { return nvgTransRGBAf(c, a); }
NVGcolor nvg_lerp_rgba(NVGcolor a, NVGcolor b, float t) { return nvgLerpRGBA(a, b, t); }
NVGcolor nvg_hsl(float h, float s, float l) { return nvgHSL(h, s, l); }
void nvg_StrokeWidth(NVGcontext *nvg, float w) { nvgStrokeWidth(nvg, w); }
void nvg_GlobalAlpha(NVGcontext *nvg, float a) { nvgGlobalAlpha(nvg, a); }
void nvg_Translate(NVGcontext *nvg, float x, float y) { nvgTranslate(nvg, x, y); }
void nvg_Scale(NVGcontext *nvg, float x, float y) { nvgScale(nvg, x, y); }
void nvg_StrokeColor(NVGcontext *nvg, NVGcolor col) { nvgStrokeColor(nvg, col); }
void nvg_FillColor(NVGcontext *nvg, NVGcolor col) { nvgFillColor(nvg, col); }
void nvg_FillPaint(NVGcontext *nvg, NVGpaint p) { nvgFillPaint(nvg, p); }
NVGpaint nvg_linear_gradient(NVGcontext *nvg, float sx, float sy, float ex, float ey, NVGcolor i, NVGcolor o) {
    return nvgLinearGradient(nvg, sx, sy, ex, ey, i, o);
}
NVGpaint nvg_image_pattern(NVGcontext *nvg, float ox, float oy, float ex, float ey, float angle, int image, float alpha) {
    return nvgImagePattern(nvg, ox, oy, ex, ey, angle, image, alpha);
}
void nvg_Scissor(NVGcontext *nvg, float x, float y, float w, float h) { nvgScissor(nvg, x, y, w, h); }
void nvg_BeginPath(NVGcontext *nvg) { nvgBeginPath(nvg); }
void nvg_MoveTo(NVGcontext *nvg, float x, float y) { nvgMoveTo(nvg, x, y); }
void nvg_LineTo(NVGcontext *nvg, float x, float y) { nvgLineTo(nvg, x, y); }
void nvg_Rect(NVGcontext *nvg, float x, float y, float w, float h) { nvgRect(nvg, x, y, w, h); }
void nvg_RoundedRect(NVGcontext *nvg, float x, float y, float w, float h, float r) { nvgRoundedRect(nvg, x, y, w, h, r); }
void nvg_RoundedRectVarying(NVGcontext *nvg, float x, float y, float w, float h, float tl, float tr, float br, float bl) {
    nvgRoundedRectVarying(nvg, x, y, w, h, tl, tr, br, bl);
}
void nvg_Circle(NVGcontext *nvg, float x, float y, float r) { nvgCircle(nvg, x, y, r); }
void nvg_Fill(NVGcontext *nvg) { nvgFill(nvg); }
void nvg_Stroke(NVGcontext *nvg) { nvgStroke(nvg); }
int nvg_create_font_mem(NVGcontext *nvg, const char *name, unsigned char *data, int n, int freeData) {
    return nvgCreateFontMem(nvg, name, data, n, freeData);
}
void nvg_FontSize(NVGcontext *nvg, float size) { nvgFontSize(nvg, size); }
void nvg_TextAlign(NVGcontext *nvg, int align) { nvgTextAlign(nvg, align); }
void nvg_FontFaceId(NVGcontext *nvg, int font) { nvgFontFaceId(nvg, font); }
float nvg_text(NVGcontext *nvg, float x, float y, const char *s, const char *end) { return nvgText(nvg, x, y, s, end); }
float nvg_text_bounds(NVGcontext *nvg, float x, float y, const char *s, const char *end, float *bounds) {
    return nvgTextBounds(nvg, x, y, s, end, bounds);
}
NVGLUframebuffer *nvglu_create_framebuffer(NVGcontext *c, int w, int h, int flags) { return nvgluCreateFramebuffer(c, w, h, flags); }
void nvg_wrap_delete_framebuffer(NVGLUframebuffer *fb) { nvgluDeleteFramebuffer(fb); }
void nvg_wrap_bind_framebuffer(NVGLUframebuffer *fb) { nvgluBindFramebuffer(fb); }
}
