#include "car_entity.h"
#include "../gfx.h"

struct car_noises_struct car_noises;

void car_entity_init(car_entity_t* ent)
{
    ent->throttle = 0.f;
    ent->brake = 0.f;
    ent->gear = 1;
    ent->clutch = 1;
    ent->engine_force = 0.f;
    ent->pre_wheels_speed = 0.f;
    ent->grip = 1.f;
    ent->engine_sound = NULL;
}

#define ENGINE_SOUND_LOOP_LEN (AUDIO_SAMPLE_RATE / 8)

float rpm_curve(float rpm)
{
    return 0.1f + fminf(rpm / 40.f, 1.f);
}

void car_entity_tick(car_entity_t* ent)
{
    vec3f car_dir = VEC3F(cosf(ent->rotation.z), sinf(ent->rotation.z), 0.f);
    
    float speed = vec3f_len(ent->velocity);
    if (vec3f_dot(car_dir, vec3f_normalized(ent->velocity)) < 0) speed = -speed;

    /*
    if (sys_is_key_just_pressed('Z')) {
        ent->gear = (ent->gear == 0) ? (0) : (ent->gear - 1);
    }
    else if (sys_is_key_just_pressed('X')) {
        ent->gear = (ent->gear == ent->cardata->gears_cnt - 1) ? (ent->gear) : (ent->gear + 1);
    }*/

    if (sys_is_key_pressed('W'))
        ent->throttle = fminf(1.f, ent->throttle + 0.2f);
    else
        ent->throttle = 0.f;

    if (sys_is_key_pressed(KEY_SPACE)) {
        ent->brake = 1.f;
    }
    else {
        ent->brake = 0;
    }

    if (sys_is_key_pressed('A'))
        ent->rotation_velocity.z -= 0.001f * speed;
    if (sys_is_key_pressed('D'))
        ent->rotation_velocity.z += 0.001f * speed;

    if (sys_is_key_just_pressed('V'))
        vec3f_add(&ent->velocity, vec3f_prod(car_dir, 10.f));

    float fuel = ent->throttle * 2;
    
    vec3f_sub(&ent->velocity, vec3f_prod(ent->velocity, ent->brake * 0.1f));
    speed = vec3f_len(ent->velocity);
    if (vec3f_dot(car_dir, vec3f_normalized(ent->velocity)) < 0) speed = -speed;

    float speed_grip = fabsf(ent->engine_force - speed) / 30.f;
    float angle_grip = fabsf(ent->rotation_velocity.z) * 2.f;
    ent->grip = 1.f - fminf(speed_grip + angle_grip, 0.95f);

    ent->engine_force += rpm_curve(ent->engine_force) * fuel - ent->engine_force * 0.03f;
    ent->engine_force = fmaxf(ent->grip * (0.5f * ent->engine_force + 0.5f * speed) + (1 - ent->grip) * ent->engine_force, 0.f);

    ent->velocity = vec3f_sum(vec3f_prod(ent->velocity, 1 - ent->grip), vec3f_prod(car_dir, ent->engine_force * ent->grip));
    
    speed = vec3f_len(ent->velocity);
    if (vec3f_dot(car_dir, vec3f_normalized(ent->velocity)) < 0) speed = -speed;
    
    vec3f_add(&ent->pos, ent->velocity);
    vec3f_mul(&ent->velocity, ent->grip * 0.99f + (1 - ent->grip) * 0.95f);

    vec3f_add(&ent->rotation, ent->rotation_velocity);
    vec3f_mul(&ent->rotation_velocity, 0.9f);

    ent->engine_sound->speed = ent->engine_force / 12.f + 0.7f;
    ent->extra_sound->volume = fmaxf(0.f, (1.f - ent->grip) * 0.3f - 0.1f);
}

void car_entity_draw(car_entity_t* ent)
{
    mat4 ident = MAT4_IDENTITY;
    mat4 modelmat = MAT4_IDENTITY;

    vec3f future_pos = vec3f_sum(ent->pos, ent->velocity);
    vec3f future_rotation = vec3f_sum(ent->rotation, ent->rotation_velocity);
    float k1 = 1 - sys.interpolation;
    float k2 = sys.interpolation;

    vec3f visual_pos = vec3f_sum(vec3f_prod(ent->pos, k1), vec3f_prod(future_pos, k2));
    vec3f visual_rotation = vec3f_sum(vec3f_prod(ent->rotation, k1), vec3f_prod(future_rotation, k2));

    vec3f car_dir = VEC3F(cosf(ent->rotation.z), sinf(ent->rotation.z), 0.f);
    float offset = ((rand() % 1000) / 500.f - 1.f) * ent->engine_force / ent->cardata->engine_force_max;
    vec3f_add(&visual_pos, VEC3F(car_dir.y * offset, car_dir.x * offset, 0.f));

    mat4_translate(&modelmat, visual_pos);
    mat4_rotate_z(&modelmat, visual_rotation.z);
    mat4_translate(&modelmat, VEC3F(-18, -38, 0));
    mat4_scale(&modelmat, VEC3F(124, 78, 1));

    glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
    glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, ident.v);

    glBindTexture(GL_TEXTURE_2D, ent->cardata->tx);
    glBindVertexArray(shader.quad_vao);
    glDrawArrays(GL_QUADS, 0, 4);
}

void car_entity_deinit(car_entity_t* ent)
{
    audio_stop_instance(&ent->engine_sound);
    audio_stop_instance(&ent->extra_sound);
}

void car_entity_set_model(car_entity_t* ent, car_model_t* car_model)
{
    if (ent->engine_sound) {
        audio_stop_instance(ent->engine_sound);
    }
    ent->engine_sound = audio_play_sample(&car_model->engine_sound_sample, 0, car_model->engine_sound_sample.len, 0.2f, 0.f, AUDIO_PLAY_REPEAT);
    ent->extra_sound = audio_play_sample(&car_noises.tire_screech, 0, car_noises.tire_screech.len, 0.f, 1.f, AUDIO_PLAY_REPEAT);
    ent->cardata = car_model;
}

FINALIZE_ENTITY_TYPE(car_entity, car_entity_init, car_entity_deinit, car_entity_draw, car_entity_tick);
