#ifndef RENG_MATH_H
#define RENG_MATH_H

#include <math.h>

typedef union vec2f {
    struct { float x, y; };
    struct { float w, h; };
    float v[2];
} vec2f;

#define VEC2F(X, Y) ((vec2f) { .x = (X), .y = (Y) })

static inline vec2f vec2f_prod (vec2f a, float b) { return VEC2F(a.x * b, a.y * b); }
static inline vec2f vec2f_qout (vec2f a, float b) { return VEC2F(a.x / b, a.y / b); }
static inline vec2f vec2f_sum  (vec2f a, vec2f b) { return VEC2F(a.x + b.x, a.y + b.y); }
static inline vec2f vec2f_diff (vec2f a, vec2f b) { return VEC2F(a.x - b.x, a.y - b.y); }
static inline vec2f vec2f_neg  (vec2f a)          { return VEC2F(-a.x, -a.y); }

static inline void vec2f_mul(vec2f* a, float b) { a->x *= b; a->y *= b; }
static inline void vec2f_div(vec2f *a, float b) { a->x /= b; a->y /= b; }
static inline void vec2f_add(vec2f *a, vec2f b) { a->x += b.x; a->y += b.y; }
static inline void vec2f_sub(vec2f *a, vec2f b) { a->x -= b.x; a->y -= b.y; }

typedef union vec3f {
    struct { float x, y, z; };
    struct { float r, g, b; };
    float v[3];
} vec3f;

#define VEC3F(X, Y, Z) ((vec3f) { .x = (X), .y = (Y), .z = (Z) })

static inline vec3f vec3f_prod(vec3f a, float b) { return VEC3F(a.x * b, a.y * b, a.z * b); }
static inline vec3f vec3f_quot(vec3f a, float b) { return VEC3F(a.x / b, a.y / b, a.z / b); }
static inline vec3f vec3f_sum(vec3f a, vec3f b) { return VEC3F(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vec3f vec3f_diff(vec3f a, vec3f b) { return VEC3F(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline vec3f vec3f_neg(vec3f a) { return VEC3F(-a.x, -a.y, -a.z); }
static inline float vec3f_dot(vec3f a, vec3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline vec3f vec3f_normalized(vec3f a)
{
    float len = sqrtf(a.x * a.x + a.y * a.y);
    if (!len) return VEC3F(0.f, 0.f, 0.f);
    return VEC3F(a.x / len, a.y / len, a.z / len);
}

static inline float vec3f_len(vec3f a) { return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z); }

static inline void vec3f_mul(vec3f* a, float b) { a->x *= b; a->y *= b; a->z *= b; }
static inline void vec3f_div(vec3f* a, float b) { a->x /= b; a->y /= b; a->z /= b; }
static inline void vec3f_add(vec3f* a, vec3f b) { a->x += b.x; a->y += b.y; a->z += b.z; }
static inline void vec3f_sub(vec3f* a, vec3f b) { a->x -= b.x; a->y -= b.y; a->z -= b.z; }

typedef union vec4f {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    float v[4];
} vec4f, rgbaf;

#define VEC4F(X, Y, Z, W) ((vec4f) { .x = (X), .y = (Y), .z = (Z), .w = (W) })

typedef struct mat4 {
    float v[16];
} mat4;

#define MAT4_IDENTITY { { 1.f, 0.f, 0.f, 0.f,   0.f, 1.f, 0.f, 0.f,    0.f, 0.f, 1.f, 0.f,    0.f, 0.f, 0.f, 1.f } }

mat4 *mat4_mul(mat4 *res, mat4 *a, mat4 *b);
mat4 *mat4_perspective(mat4 *res, float fov, float W, float H, float N, float F);
mat4 *mat4_rotation_x(mat4 *res, float ang);
mat4 *mat4_rotation_y(mat4 *res, float ang);
mat4 *mat4_rotation_z(mat4 *res, float ang);
mat4 *mat4_scaling(mat4 *res, vec3f a);
mat4 *mat4_translation(mat4 *res, vec3f a);
mat4 *mat4_ortho_projection(mat4 *res, float left, float right, float top, float bottom, float zn, float zf);
vec3f vec3f_apply_mat4(vec3f a, mat4 *b);

static inline mat4 *mat4_rotate_x(mat4 *m, float ang) {
    mat4 tmp, mcpy = *m;
    mat4_mul(m, &mcpy, mat4_rotation_x(&tmp, ang));
    return m;
}

static inline mat4 *mat4_rotate_y(mat4 *m, float ang) {
    mat4 tmp, mcpy = *m;
    mat4_mul(m, &mcpy, mat4_rotation_y(&tmp, ang));
    return m;
}

static inline mat4 *mat4_rotate_z(mat4 *m, float ang) {
    mat4 tmp, mcpy = *m;
    mat4_mul(m, &mcpy, mat4_rotation_z(&tmp, ang));
    return m;
}

static inline mat4 *mat4_scale(mat4 *m, vec3f a) {
    mat4 tmp, mcpy = *m;
    mat4_mul(m, &mcpy, mat4_scaling(&tmp, a));
    return m;
}

static inline mat4 *mat4_translate(mat4 *m, vec3f a) {
    mat4 tmp, mcpy = *m;
    mat4_mul(m, &mcpy, mat4_translation(&tmp, a));
    return m;
}

#endif