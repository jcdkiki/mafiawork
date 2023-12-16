#ifndef RENG_GFX_H
#define RENG_GFX_H

#include "def.h"

#include "sys.h"
#include "exmath.h"

typedef unsigned int textureid_t;

typedef struct font {
    textureid_t tx;
    int start_letter; // ' '
    int row_len; // 20
    int col_len; // 5
    vec3f letter_size; // z component is ignored)

    mat4 modelmat, texmat;
} font_t;

/* Does not allocate anything. you`re free to leave it "undestroyed"  */
void font_create(font_t* f, textureid_t tx, int start_letter, int row_len, int col_len, vec3f letter_size);
vec2f font_measure_text(font_t* f, const char* text);

typedef struct glvertex {
    vec3f pos;
    vec2f uv;
    rgbaf color;
} glvertex_t;

typedef struct shader {
    GLuint program;
    GLuint quad_vbo, quad_vao;
    GLuint model_mat_location;
    GLuint view_mat_location;
    GLuint proj_mat_location;
    GLuint color_location;
    GLuint textransform_mat_location;
    GLuint white_texture;
} shader_t;

enum {
    TEXTURE_LINEAR_FILTER,
    TEXTURE_NEAREST_FILTER
};

extern shader_t shader;

#define GL_EXT_MACRO(x, caps) extern PFN##caps##PROC x;
#include "gl_extensions.h"

void            gfx_init();
void            gfx_deinit();
void            gfx_draw_2d_texture_rect(textureid_t tx, float x, float y, float sx, float sy, float txx, float txy, float txscalex, float txscaley);
void            gfx_draw_2d_texture(textureid_t tx, float x, float y, float sx, float sy);
void            gfx_draw_text(char *str, font_t *font, vec3f pos, vec3f color);
void            gfx_setup_xy_screen_matrices();
textureid_t     gfx_cache_texture(char *name, unsigned int filter);
void            gfx_uncache_texture(char *name);
textureid_t     gfx_load_texture(char *name, unsigned int filter);

#endif