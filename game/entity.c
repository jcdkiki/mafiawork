#include "def.h"
#include "entity.h"
#include "exmath.h"
#include "gfx.h"

void entity_empty_func(base_entity_t* ent) {}
FINALIZE_ENTITY_TYPE(base_entity, entity_empty_func, entity_empty_func, entity_empty_func, entity_empty_func);

list_t entlist;

base_entity_t* entity_create(entity_vtable_t* type) {
    base_entity_t* ent = sys_malloc(type->sz);
    memset(ent, 0, type->sz);
    ent->type = type;
    ent->type->init(ent);
    
    return ent;
}

void entity_tick(base_entity_t *ent)
{
    if (ent->last_tick == sys.tick) return;
    ent->last_tick = sys.tick;
    ent->type->tick(ent);
}

void entity_destroy(base_entity_t* ent)
{
    ent->type->deinit(ent);
    sys_free(ent);
}