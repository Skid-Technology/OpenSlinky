// Chest ESP - draw boxes around chests (and ender chests, trapped chests, beacons) through walls.
//
// Every tick the module collects the containers within "range" (at most 100); every frame it
// draws a box around each, in the colour for its type ("filled" also fills it).
//
// Callbacks (descriptor slots +0x48 / +0xa8):
//   chest_esp_on_tick          every tick: collect the containers
//   chest_esp_on_render_world  every frame: draw them
#include "slinky/engine.h"

struct Vec3 { double x, y, z; };
struct Found { Vec3 pos; int id; int padding_1c; };   // one container (0x20 bytes)

// ---- settings ----
extern "C" float chest_esp_range_value;                    // "range"
extern "C" unsigned char chest_esp_render_chest;           // "render_chest"
extern "C" unsigned char chest_esp_render_ender_chest;     // "render_ender_chest"
extern "C" unsigned char chest_esp_render_trapped_chest;   // "render_trapped_chest"
extern "C" unsigned char chest_esp_render_beacon;          // "render_beacon"
extern "C" float chest_esp_line_width_value;               // "line_width"
extern "C" float chest_esp_color_chest_value[];            // "color_chest" (r, g, b)
extern "C" float chest_esp_color_ender_chest_value[];      // "color_ender_chest"
extern "C" float chest_esp_color_trapped_chest_value[];    // "color_trapped_chest"
extern "C" float chest_esp_color_beacon_value[];           // "color_beacon"
extern "C" unsigned char chest_esp_filled_value;           // "filled"

// ---- module state ----
extern "C" unsigned long long chest_esp_found_count;
extern "C" Found chest_esp_chests[100];

// ---- OpenGL ----
extern "C" void (**ptr_to_pGlLineWidth)(float);
extern "C" void (**ptr_to_pGlColor3f)(float, float, float);
extern "C" void (**ptr_to_pGlPopMatrix)();
extern "C" void (**ptr_to_pGlPushMatrix)();
extern "C" void (**ptr_to_pGlTranslatef)(float, float, float);
extern "C" void (**ptr_to_pGlScalef)(float, float, float);
extern "C" void (**ptr_to_pGlColor4f)(float, float, float, float);
FN(void, gl_array_xyz_float, (void *h));        // set up the unit box (-1..1) vertex array
FN(void, gl_array_color1, ());                  // draw it filled
FN(void, gl_array_color2, ());                  // draw its outline
FN(void, disable_vertex_array_state, ());

// ---- game / engine functions ----
FN(Vec3, java_get_entity_pos_x, (void *jvm, void *entity));
FN(void *, java_get_world_loaded_tile_entity_list, (void *jvm, void *world));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(void *, java_get_tile_entity_block_type, (void *jvm, void *te));
FN(int, get_block_id, (void *jvm, void *blk));
FN(Vec3, java_get_block_pos_as_vec3, (void *jvm, void *te));
FN(double, vec3_distance_sq, (Vec3 a, Vec3 b));
FN(Vec3, vec3_sub, (Vec3 a, Vec3 b));
FN(Vec3, vec3_add, (Vec3 a, Vec3 b));

enum { BLOCK_CHEST = 54, BLOCK_ENDER_CHEST = 130, BLOCK_BEACON = 138, BLOCK_TRAPPED_CHEST = 146 };

// Should this kind of block be shown?
static bool wanted(int block_id) {
    switch (block_id) {
    case BLOCK_CHEST: return chest_esp_render_chest;
    case BLOCK_ENDER_CHEST: return chest_esp_render_ender_chest;
    case BLOCK_BEACON: return chest_esp_render_beacon == 1;
    case BLOCK_TRAPPED_CHEST: return chest_esp_render_trapped_chest;
    default: return false;
    }
}

// Every tick: collect the containers in range.
extern "C" void chest_esp_on_tick(Hook *hook) {
    chest_esp_found_count = 0;
    Ctx *ctx = hook->ctx;
    if (!ctx->world)
        return;
    Vec3 my_pos = java_get_entity_pos_x(ctx->jvm, ctx->player);
    void *tile_entities = java_get_world_loaded_tile_entity_list(ctx->jvm, ctx->world);
    for (int i = 0; i < java_call_list_size(ctx->jvm, tile_entities); i++) {
        void *tile_entity = java_call_list_get(ctx->jvm, tile_entities, i);
        void *block = java_get_tile_entity_block_type(ctx->jvm, tile_entity);
        if (!block)
            continue;
        int block_id = get_block_id(ctx->jvm, block);
        if (!wanted(block_id))
            continue;
        Vec3 pos = java_get_block_pos_as_vec3(ctx->jvm, tile_entity);
        if (vec3_distance_sq(pos, my_pos) > (double)(chest_esp_range_value * chest_esp_range_value))
            continue;
        unsigned long long n = chest_esp_found_count;
        chest_esp_found_count = n + 1;
        chest_esp_chests[n].pos = pos;
        chest_esp_chests[n].id = block_id;
        if (n + 1 == 100)
            return;   // list full
    }
}

// Every frame: a box around each container. h + 8 holds the camera position.
extern "C" void chest_esp_on_render_world(char *h) {
    gl_array_xyz_float(h);
    (*ptr_to_pGlLineWidth)(chest_esp_line_width_value);
    Vec3 *camera = (Vec3 *)(h + 8);
    for (unsigned long long i = 0; i < chest_esp_found_count; i++) {
        Found *found = &chest_esp_chests[i];
        float *colour;
        if (found->id == BLOCK_ENDER_CHEST)
            colour = chest_esp_color_ender_chest_value;
        else if (found->id == BLOCK_BEACON)
            colour = chest_esp_color_beacon_value;
        else if (found->id == BLOCK_TRAPPED_CHEST)
            colour = chest_esp_color_trapped_chest_value;
        else
            colour = chest_esp_color_chest_value;

        // Centre of the block, relative to the camera; the unit box scaled to 1x1x1.
        Vec3 centre = vec3_add(vec3_sub(found->pos, *camera), Vec3{0.5, 0.5, 0.5});
        (*ptr_to_pGlPushMatrix)();
        (*ptr_to_pGlTranslatef)((float)centre.x, (float)centre.y, (float)centre.z);
        (*ptr_to_pGlScalef)(0.5f, 0.5f, 0.5f);
        if (chest_esp_filled_value == 1) {
            (*ptr_to_pGlColor4f)(colour[0], colour[1], colour[2], 0.4f);
            gl_array_color1();
        }
        (*ptr_to_pGlColor3f)(colour[0], colour[1], colour[2]);
        gl_array_color2();
        (*ptr_to_pGlPopMatrix)();
    }
    disable_vertex_array_state();
}
