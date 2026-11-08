// Block ESP - outline chosen blocks (ores, ...) through walls.
//
// The blocks in "allowed_blocks" are found per chunk; each chunk gets a mesh of box outlines
// (built by the engine, rebuilt when blocks change). Every frame the meshes of the chunks
// within "range" are drawn, optionally limited to "block_render_limit" boxes per chunk.
//
// Callbacks (descriptor slots +0x48 / +0x98 / +0xa8 / +0x188 / +0x1e8):
//   block_esp_on_tick          every tick: rebuild meshes when needed
//   block_esp_on_toggle        switched on / off
//   block_esp_on_render_world  every frame: draw
//   block_esp_on_world_change  a block changed
//   block_esp_on_shutdown      module unloaded: free everything
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Buf { void *p; unsigned long long n, cap; };   // a growable array
struct Mesh {                    // one chunk (0x50 bytes)
    int cx, cz;                  // chunk coordinates
    unsigned long long a, b;     // +0x08
    Buf lines;                   // +0x18
    Buf verts;                   // +0x30 vertices, 16 bytes each: x, y, z (float) + r, g, b (bytes)
    unsigned short *idx;         // +0x48 line indices
};
struct MeshVec { unsigned long long size, cap; Mesh *data; };

struct EvHook {
    char padding_00[8];
    Ctx *ctx;            // +0x08 game context
    void *ev;            // +0x10 module descriptor that was toggled
    unsigned char on;    // +0x18 1 = switched on
};

// ---- settings ----
extern "C" float block_esp_line_width_value;               // "line_width"
extern "C" float block_esp_block_render_limit_value;       // "block_render_limit": boxes per chunk
extern "C" unsigned char block_esp_should_limit_block_render;   // "should_limit_block_render"
extern "C" float block_esp_range_value;                    // "range" (in chunks)
extern "C" unsigned char block_esp_allowed_blocks_changed; // "allowed_blocks" was edited

// ---- module state ----
extern "C" char block_esp_module_descriptor[];
extern "C" MeshVec block_esp_mesh_count_capacity;   // mesh count / capacity
extern "C" Mesh *block_esp_meshes;
extern "C" unsigned char block_esp_rebuild_requested;
extern "C" unsigned long long id_table_count, id_table_capacity;
extern "C" void *id_table_entries;
extern "C" void *block_esp_block_scan_callback;     // how to read a chunk's blocks (depends on the game version)

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlEnableClientState)(unsigned);
extern "C" void (**ptr_to_pGlDisableClientState)(unsigned);
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlVertexPointer)(int, unsigned, int, const void *);
extern "C" void (**ptr_to_pGlColorPointer)(int, unsigned, int, const void *);
extern "C" void (**ptr_to_pGlDrawElements)(unsigned, unsigned long long, unsigned, const void *);
enum { GL_LINES = 1, GL_UNSIGNED_BYTE = 0x1401, GL_UNSIGNED_SHORT = 0x1403, GL_FLOAT = 0x1406, GL_VERTEX_ARRAY = 0x8074, GL_COLOR_ARRAY = 0x8076 };

// ---- engine functions ----
FN(void, rebuild_block_esp_mesh, (Mesh *m));
FN(int, java_call_object_hash_code, (void *jvm, void *world));
FN(void, block_esp_queue_loaded_chunks, (Ctx *ctx));
FN(void, libc_free, (void *ptr));
FN(double, libc_hypot, (double x, double z));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(void, equipment_event, (void *x));
FN(void, scan_16x16x16_chunk_section_searched, ());
FN(void, scan_16_bit_block_data, ());

// Free all meshes.
static void clear_meshes() {
    for (unsigned long long i = 0; i < block_esp_mesh_count_capacity.size; i++) {
        Mesh *mesh = &block_esp_meshes[i];
        mesh->a = mesh->b = 0;
        libc_free(mesh->lines.p);
        mesh->lines.p = nullptr;
        mesh->lines.n = 0;
        mesh->lines.cap = 0;
        libc_free(mesh->verts.p);
        mesh->verts.p = nullptr;
        mesh->verts.n = 0;
        mesh->verts.cap = 0;
        libc_free(mesh->idx);
        mesh->idx = nullptr;
    }
    block_esp_mesh_count_capacity.size = block_esp_mesh_count_capacity.cap = 0;
    libc_free(block_esp_meshes);
    block_esp_meshes = nullptr;
}

extern "C" void block_esp_on_tick(Hook *hook) {
    Ctx *ctx = hook->ctx;
    // The block list changed: rebuild every mesh.
    if (ctx->in_game == 1 && block_esp_allowed_blocks_changed) {
        for (unsigned long long i = 0; i < block_esp_mesh_count_capacity.size; i++)
            rebuild_block_esp_mesh(&block_esp_meshes[i]);
        block_esp_allowed_blocks_changed = 0;
    }
    if (!ctx->world) {
        clear_meshes();
        return;
    }
    java_call_object_hash_code(ctx->jvm, ctx->world);   // (result unused)
    if (block_esp_rebuild_requested == 1) {
        clear_meshes();
        block_esp_queue_loaded_chunks(ctx);
        block_esp_rebuild_requested = 0;
    }
}

extern "C" void block_esp_on_toggle(EvHook *hook) {
    if (hook->ev != block_esp_module_descriptor)
        return;
    if (!hook->on) {
        clear_meshes();
        return;
    }
    // Switched on: pick the chunk reader for this game version, and scan the loaded chunks.
    Ctx *ctx = hook->ctx;
    block_esp_block_scan_callback = **(int **)ctx->jvm == 0 ? (void *)scan_16x16x16_chunk_section_searched : (void *)scan_16_bit_block_data;
    if (ctx->world)
        block_esp_queue_loaded_chunks(ctx);
}

// Every frame. h + 8 holds the camera position.
extern "C" void block_esp_on_render_world(char *h) {
    if (!block_esp_mesh_count_capacity.size)
        return;
    (*ptr_to_pGlEnableClientState)(GL_VERTEX_ARRAY);
    (*ptr_to_pGlEnableClientState)(GL_COLOR_ARRAY);
    (*ptr_to_pGlLineWidth)(block_esp_line_width_value);

    // A box is 12 lines = 24 indices.
    unsigned long long index_limit = (unsigned long long)(block_esp_block_render_limit_value * 24.0f);
    unsigned long long limit = block_esp_should_limit_block_render ? index_limit : 0x1000;

    Vec3 *camera = (Vec3 *)(h + 8);
    double camera_chunk_x = camera->x * 0.0625;   // blocks -> chunks (1/16)
    double camera_chunk_z = 0.0625 * camera->z;
    for (unsigned long long i = 0; i < block_esp_mesh_count_capacity.size; i++) {
        Mesh *mesh = &block_esp_meshes[i];
        // Only chunks within "range" (from the chunk's centre).
        double dx = camera_chunk_x - ((double)mesh->cx + 0.5);
        double dz = camera_chunk_z - ((double)mesh->cz + 0.5);
        if (libc_hypot(dx, dz) > (double)block_esp_range_value)
            continue;
        (*ptr_to_pGlPushMatrix)();
        Vec3 corner = {(double)(mesh->cx << 4), 0.0, (double)(mesh->cz << 4)};
        Vec3 offset = vec3_sub(corner, *camera);
        (*ptr_to_pGlTranslatef)((float)offset.x, (float)offset.y, (float)offset.z);
        (*ptr_to_pGlVertexPointer)(3, GL_FLOAT, 0x10, mesh->verts.p);
        (*ptr_to_pGlColorPointer)(3, GL_UNSIGNED_BYTE, 0x10, (char *)mesh->verts.p + 0xc);
        unsigned long long count = mesh->verts.n;
        (*ptr_to_pGlDrawElements)(GL_LINES, limit < count ? limit : count, GL_UNSIGNED_SHORT, mesh->idx);
        (*ptr_to_pGlPopMatrix)();
    }

    (*ptr_to_pGlDisableClientState)(GL_VERTEX_ARRAY);
    (*ptr_to_pGlDisableClientState)(GL_COLOR_ARRAY);
}

extern "C" void block_esp_on_world_change(char *h) {
    equipment_event(h + 8);
}

extern "C" void block_esp_on_shutdown(Hook *hook) {
    id_table_count = id_table_capacity = 0;
    libc_free(id_table_entries);
    id_table_entries = nullptr;
    clear_meshes();
}
