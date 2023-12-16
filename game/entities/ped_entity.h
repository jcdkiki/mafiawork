#ifndef RENG_PED_ENTITY_H
#define RENG_PED_ENTITY_H

#include "../def.h"
#include "../entity.h"
#include "car_entity.h"

typedef struct ped_type {
	uint32_t texture;
} ped_type_t;

extern entity_vtable_t ped_entity_vtable;
typedef struct ped_entity {
	EXTEND_BASE_ENTITY;
	
	car_entity_t* car;			/* NULL IF ONFOOT */
	ped_type_t* pedtype;
} ped_entity_t;

void ped_entity_set_type(ped_entity_t* ped, ped_type_t* type);

#endif