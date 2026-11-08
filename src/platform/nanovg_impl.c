// NanoVG (third_party/nanovg, zlib licence) with its GL3 backend, drawing through the client's GL slots.
// The image linked exactly this library; the client calls it by address (src/platform/nvg_names.cpp).
#define SLINKY_GL_MACROS
#include "slinky/gl_slots.h"
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"
