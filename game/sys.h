#ifndef RENG_CORE_H
#define RENG_CORE_H

#include "def.h"
#include "exmath.h"

#ifdef RENG_MEMTRACE
    void *sys_internal_malloc(size_t size, const char *file, int line);
    void  sys_internal_free(void *ptr, const char *file, int line);
    void *sys_internal_realloc(void *ptr, size_t size, const char *file, int line); 

    void gl_internal_gen_buffers(GLsizei n, GLuint *ptr, const char *file, int line);
    void gl_internal_gen_textures(GLsizei n, GLuint *ptr, const char *file, int line);
    void gl_internal_gen_vertex_arrays(GLsizei n, GLuint *ptr, const char *file, int line);

    void gl_internal_delete_buffers(GLsizei n, GLuint *ptr, const char *file, int line);
    void gl_internal_delete_textures(GLsizei n, GLuint *ptr, const char *file, int line);
    void gl_internal_delete_vertex_arrays(GLsizei n, GLuint *ptr, const char *file, int line);

    #define sys_malloc(sz)                  sys_internal_malloc(sz, __FILE__, __LINE__)
    #define sys_free(p)                     sys_internal_free(p, __FILE__, __LINE__)
    #define sys_realloc(p, sz)              sys_internal_realloc(p, sz, __FILE__, __LINE__)

    #define glwrapGenBuffers(n, ptr)           gl_internal_gen_buffers(n, ptr, __FILE__, __LINE__)
    #define glwrapGenTextures(n, ptr)          gl_internal_gen_textures(n, ptr, __FILE__, __LINE__)
    #define glwrapGenVertexArrays(n, ptr)      gl_internal_gen_vertex_arrays(n, ptr, __FILE__, __LINE__)

    #define glwrapDeleteBuffers(n, ptr)        gl_internal_delete_buffers(n, ptr, __FILE__, __LINE__)
    #define glwrapDeleteTextures(n, ptr)       gl_internal_delete_textures(n, ptr, __FILE__, __LINE__)
    #define glwrapDeleteVertexArrays(n, ptr)   gl_internal_delete_vertex_arrays(n, ptr, __FILE__, __LINE__)
#else
    #define sys_malloc   malloc
    #define sys_free     free
    #define sys_realloc  realloc

    #define glwrapGenBuffers glGenBuffers
    #define glwrapGenTextures glGenTextures
    #define glwrapGenVertexArrays glGenVertexArrays

    #define glwrapDeleteBuffers glDeleteBuffers
    #define glwrapDeleteTextures glDeleteTextures
    #define glwrapDeleteVertexArrays glDeleteVertexArrays
#endif

#define UTILS_MALLOC(sz) sys_malloc(sz)
#define UTILS_REALLOC(ptr, newsz) sys_realloc(ptr, newsz)
#define UTILS_FREE(ptr) sys_free(ptr)
#define UTILS_VECTOR3 vec3f
#define UTILS_VECTOR2 vec2f
#include "utils.h"

#include "exmath.h"

typedef struct sys_common {
    int tick;
    int width, height;
    int time;
    float interpolation;
    bool running;
    int64_t mem_usage;

    vec2f mouse;
    vec2f mouse_delta;
} sys_common_t;

typedef enum {
    FILEPOS_SET,
    FILEPOS_ADD
} FILEPOS;

typedef uintmax_t file_handle_t;

extern sys_common_t sys;

int             sys_is_key_pressed(int key);
int             sys_is_key_just_pressed(int key);
int             sys_is_key_released(int key);
void            sys_close_window();
void            sys_fatal_error(const char* msg);
file_handle_t   sys_open_file(const char* name, const char* openflags);
file_handle_t   sys_reopen_file(file_handle_t file, const char* name, const char* openflags);
void            sys_close_file(file_handle_t file);
size_t          sys_read_file(file_handle_t file, void *dst, size_t bytes);
size_t          sys_get_file_pos(file_handle_t file);
void            sys_set_file_pos(file_handle_t file, size_t offset, FILEPOS type);

#ifdef RENG_ENABLE_LOG
    void sys_logf(const char *fmt, const char *file, int line, ...);
    void sys_log(const char *str, const char *file, int line);
    #define RENG_LOGF(fmt, ...) sys_logf(fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
    #define RENG_LOG(str) sys_log(str "\n", __FILE__, __LINE__)
#else
    #define RENG_LOGF(fmt, ...)
    #define RENG_LOG(str)
#endif

#endif