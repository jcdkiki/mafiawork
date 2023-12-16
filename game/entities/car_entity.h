#ifndef RENG_CAR_ENTITY_H
#define RENG_CAR_ENTITY_H

#include "../def.h"
#include "../entity.h"
#include "../audio.h"

typedef struct car_model {
    uint32_t tx;
    audio_sample_t engine_sound_sample;
    float engine_force_max;
} car_model_t;

extern entity_vtable_t car_entity_vtable;
typedef struct car_entity {
    EXTEND_BASE_ENTITY;

    car_model_t* cardata;
    float throttle;
    float brake;
    float engine_force;
    float pre_wheels_speed;
    int8_t clutch;
    float grip;
    int gear;

    audio_instance_t* engine_sound;
    audio_instance_t* extra_sound;
    float old_throttle;
} car_entity_t;

extern struct car_noises_struct {
    audio_sample_t tire_screech;
} car_noises;

void car_entity_set_model(car_entity_t* ent, car_model_t* car_model);

#endif