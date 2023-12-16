#ifndef RENG_UTILS_H
#define RENG_UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef UTILS_MALLOC
#include <stdlib.h>
#define UTILS_MALLOC(sz) malloc(sz)
#define UTILS_REALLOC(ptr, newsz) realloc(ptr, newsz)
#define UTILS_FREE(ptr) free(ptr)

typedef struct utils_vec3 {
    float x, y, z;
} utils_vec3_t;

typedef struct utils_vec2 {
    float x, y;
} utils_vec2_t;

#define UTILS_VECTOR3 utils_vec3_t
#define UTILS_VECTOR2 utils_vec2_t
#endif

#define LISTNODE_DATA(nodeptr, type) (*((type *)(nodeptr + 1)))
#define LISTNODE_DATAPTR(nodeptr, type) ((type *)(nodeptr + 1))
#define list_emplace_front(list_ptr, type) ((type*)list_emplace_front_vptr((list_ptr), sizeof(listnode_t) + sizeof(type)))
#define list_emplace_back(list_ptr, type) ((type*)list_emplace_back_vptr((list_ptr), sizeof(listnode_t) + sizeof(type)))

#define HT_SECTION_LEN 4
#define HASHBUCKET_DATA(bucket_ptr, type) (*((type*)(bucket_ptr + 1)))
#define HASHBUCKET_DATAPTR(bucket_ptr, type) ((type*)(bucket_ptr + 1))
#define hashtable_of(type) ((hashtable_t) { .n_buckets = 0, .bucketsize = sizeof(hashbucket_t) + sizeof(type), .data = NULL })

#define vector_of(type) ((vector_t) { .size = 0, .capacity = 0, .typesize = sizeof(type), .data = NULL })
#define vector_at(vec_ptr, n, type) ((type*)vector_at_vptr(vec_ptr, n))
#define vector_emplace_back(vec_ptr, type) ((type*)vector_emplace_back_vptr(vec_ptr))

#define EMPTY_STR { .size = 0, .capacity = 0, .data = NULL }

#define EMPTY_QUADTREE(left_, top_, right_, bottom_, parent_) \
    ((quadtree_t) {                     \
        .is_leaf = 1,                   \
        .left = left_,                  \
        .top = top_,                    \
        .right = right_,                \
        .bottom = bottom_,              \
        .n_children = 0,                \
        .children = { 0, 0, 0, 0 },     \
        .parent = parent_               \
    })

typedef struct listnode
{
    struct listnode* prev;
    struct listnode* next;
} listnode_t;

typedef struct list {
    size_t size;
    listnode_t* begin;
    listnode_t* end;
} list_t;

typedef struct hashbucket {
    char *name;
    int32_t used;
} hashbucket_t;

typedef struct hashtable {
    size_t n_buckets;
    size_t bucketsize;
    char *data;
} hashtable_t;

typedef struct vector {
    size_t size;
    size_t capacity;
    size_t typesize;
    char* data;
} vector_t;

typedef void (*vector_data_constructor_t)(void* self);

typedef struct str8 {
    size_t size;
    size_t capacity;
    char *data;
} str8;

typedef struct object2d {
    #define EXTEND_OBJECT2D   \
        UTILS_VECTOR3 pos;    \
        vector_t indices;     \
                              \
        struct {              \
            UTILS_VECTOR2 lt; \
            UTILS_VECTOR2 rb; \
        } bound_box

    EXTEND_OBJECT2D;
} object2d_t;

typedef struct grid2d_block {
    vector_t objects;
} grid2d_block_t;

typedef struct grid2d {
    UTILS_VECTOR2 block_size;

    uint32_t grid_width;
    uint32_t grid_height;

    grid2d_block_t* blocks;
} grid2d_t;

void object2d_init(object2d_t *obj, UTILS_VECTOR3 pos, UTILS_VECTOR2 left_top, UTILS_VECTOR2 right_bottom);

void grid2d_create(grid2d_t* grid, UTILS_VECTOR2 block_size, uint32_t w, uint32_t h);
void grid2d_destroy(grid2d_t* grid);
void grid2d_remove_object(grid2d_t* grid, object2d_t* obj);
void grid2d_insert_object(grid2d_t* grid, object2d_t* obj);
void grid2d_update_object(grid2d_t* grid, object2d_t* obj);

unsigned int    get_hash(const char* string);

void            list_destroy(list_t* list);
void            list_destroy_node(list_t* list, listnode_t* n);
void*           list_emplace_front_vptr(list_t* list, size_t node_size);
void*           list_emplace_back_vptr(list_t* list, size_t node_size);

hashbucket_t*   hashtable_pick_bucket(hashtable_t* table, size_t n);
hashbucket_t*   hashtable_next_bucket(hashtable_t* table, hashbucket_t* bucket);
hashbucket_t*   hashtable_emplace(hashtable_t* table, const char* name);
hashbucket_t*   hashtable_find(hashtable_t* table, const char* name);
void            hashtable_grow(hashtable_t* table);
void            hashtable_destroy(hashtable_t* table);

void            hashbucket_empty(hashbucket_t* bucket);

void*           vector_at_vptr(vector_t* vec, size_t n);
void            vector_resize_ub(vector_t* vec, size_t newsize);
void            vector_resize_memset(vector_t* vec, size_t newsize, int32_t fill);
void            vector_resize_fill(vector_t* vec, size_t newsize, void *fill);
void            vector_resize_ctor(vector_t* vec, size_t newsize, vector_data_constructor_t ctor);
void            vector_make_sure_fits(vector_t* vec);
void*           vector_emplace_back_vptr(vector_t* vec);
void            vector_destroy(vector_t* vec);

void            str8_create(str8 *s, size_t size);
void            str8_create_from_cstr(str8 *str, const char *cstr);
void            str8_create_by_printf(str8 *str, const char *fmt, ...);
void            str8_create_by_vprintf(str8 *str, const char *fmt, va_list args);
void            str8_resize(str8 *s, size_t newsize);
void            str8_resize_ub(str8 *s, size_t newsize);
void            str8_push_back(str8 *s, char c);
void            str8_make_sure_fits(str8 *s, size_t ind);
char*           str8_duplicate_as_cstr(str8 *s);
void            str8_fit(str8 *str);
void            str8_destroy(str8 *s);

#endif