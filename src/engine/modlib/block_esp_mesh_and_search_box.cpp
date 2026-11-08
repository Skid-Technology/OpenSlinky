// helper functions (formerly address block; names inferred from behaviour).
#include "slinky/engine.h"
FN(void, libc_free, (void *ptr));
FN(unsigned long long, libc_strlen, (const char *str));
struct Vec { unsigned long long size, cap; void *data; };
struct Lists { char padding_00[8]; Vec a, b, c; };
extern "C" void free_block_search_section_lists(Lists *l) {
    l->a.size = 0; l->a.cap = 0; libc_free(l->a.data); l->a.data = 0;
    l->b.size = 0; l->b.cap = 0; libc_free(l->b.data); l->b.data = 0;
    l->c.size = 0; l->c.cap = 0; libc_free(l->c.data); l->c.data = 0;
}
extern "C" void search_box_append_char(char *buf, char character) {
    unsigned long long n = libc_strlen(buf);
    if (n > 99) return;
    unsigned char u = (unsigned char)((character & 0xdf) - 0x41);
    asm("" : "+r"(u));
    if (u >= 0x1a) {
        unsigned char d = (unsigned char)(character - 0x30);
    asm("" : "+r"(d));
        unsigned char notdigit = d >= 10;
    asm("" : "+r"(notdigit));
        unsigned char notspace = character != 0x20;
    asm("" : "+r"(notspace));
        if (notdigit & notspace) return;
    }
    buf[n] = character;
}

FN(void *, java_get_world_chunk_provider_client, (void *jvm, void *chunk_provider_client));
FN(void *, java_get_chunk_provider_client, (void *jvm, void *chunk_provider_client));
FN(int, java_call_list_size, (void *jvm, void *list));
FN(void *, java_call_list_get, (void *jvm, void *list, int i));
FN(int, java_get_chunk_x_position, (void *jvm, void *chunk));
FN(int, java_get_chunk_z_position, (void *jvm, void *chunk));
struct Rec { void *jvm; void *world; int one; int id; int lvl; int padding_1c, padding_20, padding_24; };
FN(void, equipment_event, (Rec *r));
struct S3f { void *jvm; char padding_08[0x18]; void *world; };
extern "C" void block_esp_queue_loaded_chunks(S3f *s) {
    void *chunk_provider_client = java_get_world_chunk_provider_client(s->jvm, s->world);
    if (!chunk_provider_client) return;
    void *list = java_get_chunk_provider_client(s->jvm, chunk_provider_client);
    int n = java_call_list_size(s->jvm, list);
    if (n <= 0) return;
    for (int i = 0; i != n; i++) {
        void *element = java_call_list_get(s->jvm, list, i);
        int chunk_x_position = java_get_chunk_x_position(s->jvm, element);
        int lvl = java_get_chunk_z_position(s->jvm, element);
        Rec r;
        r.jvm = s->jvm;
        r.world = s->world;
        r.one = 1;
        r.id = chunk_x_position;
        r.lvl = lvl;
        r.padding_1c = 0; r.padding_20 = 0; r.padding_24 = 0;
        equipment_event(&r);
    }
}

// rebuild a coloured box mesh: 8 corner vertices + 24 line indices per block
FN(unsigned char, vector_grow, (void *data, unsigned long long elem, unsigned long long size, void *cap));
struct Vtx { float x, y, z; unsigned char r, g, b, a; };
struct Corner { float x, y, z; };
struct Blk { unsigned char x, y, z, padding_03; char padding_04[4]; float *color; };
struct Mesh {
    char padding_00[8]; unsigned long long nblocks; char padding_10[8]; Blk *blocks;
    unsigned long long nv, capv; Vtx *v;
    unsigned long long ni, capi; unsigned short *idx;
};
extern "C" const float factor_255_in_block_esp_mesh_and_search_box;
extern "C" const Corner cube_corner_offsets[8];
extern "C" const unsigned short cube_edge_indices[24];
extern "C" void rebuild_block_esp_mesh(Mesh *m) {
    m->nv = 0;
    m->ni = 0;
    if (!m->nblocks) return;
    for (unsigned long long b = 0; b < m->nblocks; b++) {
        Blk *bk = &m->blocks[b];
        float *col = bk->color;
        unsigned char r = (unsigned char)(int)(col[2] * factor_255_in_block_esp_mesh_and_search_box);
        unsigned char g = (unsigned char)(int)(col[3] * factor_255_in_block_esp_mesh_and_search_box);
        unsigned char bl = (unsigned char)(int)(col[4] * factor_255_in_block_esp_mesh_and_search_box);
        unsigned long long base = m->nv;
#pragma clang loop unroll(disable)
        for (int k = 0; k < 8; k++) {
            if (!vector_grow(&m->v, 0x10, m->nv, &m->capv)) return;
            Vtx *v = &m->v[m->nv];
            v->x = cube_corner_offsets[k].x + (float)bk->x;
            v->y = cube_corner_offsets[k].y + (float)bk->y;
            v->z = cube_corner_offsets[k].z + (float)bk->z;
            v->r = r; v->g = g; v->b = bl; v->a = 0xff;
            m->nv++;
        }
#pragma clang loop unroll(disable)
        for (int k = 0; k < 24; k++) {
            if (!vector_grow(&m->idx, 2, m->ni, &m->capi)) return;
            m->idx[m->ni] = (unsigned short)(cube_edge_indices[k] + base);
            m->ni++;
        }
    }
}

// ---- free the whole block-search mesh cache: every section's three lists, then the section array ----
struct LVec3f { unsigned long long size, cap; Lists *data; };
extern "C" LVec3f block_esp_mesh_count_capacity;
extern "C" void free_block_search_cache() {
    for (unsigned long long i = 0; i < block_esp_mesh_count_capacity.size; i++) {
        Lists *l = &block_esp_mesh_count_capacity.data[i];
        l->a.size = 0; l->a.cap = 0; libc_free(l->a.data); l->a.data = 0;
        l->b.size = 0; l->b.cap = 0; libc_free(l->b.data); l->b.data = 0;
        l->c.size = 0; l->c.cap = 0; libc_free(l->c.data); l->c.data = 0;
    }
    block_esp_mesh_count_capacity.size = 0;
    block_esp_mesh_count_capacity.cap = 0;
    libc_free(block_esp_mesh_count_capacity.data);
    block_esp_mesh_count_capacity.data = 0;
}
