#ifndef DOC_ENTITY_H
#define DOC_ENTITY_H

#include "def.h"
#include "sys.h"
#include "exmath.h"

#define FINALIZE_ENTITY_TYPE(ent, init_fn, deinit_fn, draw_fn, tick_fn)         \
    entity_vtable_t ent##_vtable = {                                            \
        .init = (entity_func_t)init_fn,                                         \
        .deinit = (entity_func_t)deinit_fn,                                     \
        .draw = (entity_func_t)draw_fn,                                         \
        .tick = (entity_func_t)tick_fn,                                         \
        .sz = sizeof(struct ent),                                               \
        .name = #ent                                                            \
    }

struct base_entity;
typedef void (*entity_func_t)(struct base_entity*);

typedef struct entity_vtable {
    entity_func_t init;
    entity_func_t deinit;
    entity_func_t draw;
    entity_func_t tick;

    size_t sz;
    const char *name;
} entity_vtable_t;

extern entity_vtable_t base_entity_vtable;
typedef struct base_entity {
    #define EXTEND_BASE_ENTITY                  \
        EXTEND_OBJECT2D;                        \
        entity_vtable_t *type;                  \
                                                \
        int64_t last_tick;                      \
        vec3f velocity;                         \
        vec3f rotation, rotation_velocity

    EXTEND_BASE_ENTITY;
} base_entity_t;

extern list_t entlist;

static inline void  entity_draw(base_entity_t* ent) { ent->type->draw(ent); }
void                entity_tick(base_entity_t* ent);

void                entity_empty_func(base_entity_t* ent);
base_entity_t*      entity_create(entity_vtable_t* type);
void                entity_destroy(base_entity_t* ent);
void                entity_insert_into_world(base_entity_t* ent);           /* Game engine owns the entity you inserted. No need to destroy it for you */

#endif