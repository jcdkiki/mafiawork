#ifndef RENG_RWSTREAM_H
#define RENG_RWSTREAM_H

#include "def.h"

#include <stdio.h>
#include "sys.h"
#include "gfx.h"
#include "exmath.h"

typedef struct material {
    uint32_t color;
    int32_t is_textured;
    float ambient;
    float specular;
    float diffuse;

    textureid_t tx[2];
} material_t;

typedef struct drawcall {
    GLuint ebo;
    size_t n_verts;
} drawcall_t;

typedef struct geometry {
    size_t n_triangles;
    GLuint vbo;
    GLuint vao;

    drawcall_t* draws;
    material_t* materials;
    size_t      n_draws;
    size_t      n_materials;
} geometry_t;

typedef struct atomic {
    uint32_t id;
    uint32_t frame_index;
    uint32_t geometry_index;
    uint32_t flags;
} atomic_t;

typedef struct frame {
    size_t namelen;
    char *name;

    mat4 rotation_mat;
    vec3f pos;
    uint32_t index;
    uint32_t flags;
} frame_t;

typedef struct dff {
    atomic_t*   atomics;
    geometry_t* geometries;
    frame_t* frames;
    
    size_t n_atomics;
    size_t n_geometries;
    size_t n_frames;
} dff_t;

#define EMPTY_DFF ((dff_t) { 0 } )

void dff_create_from_file(dff_t* data, FILE *s);
void dff_destroy(dff_t* dff);

void rw_cache_texture_dict(const char *name);
void rw_uncache_texture_dict(const char *name);

void rw_init();
void rw_deinit();

#endif