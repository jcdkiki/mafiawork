#include "ped_entity.h"
#include "../gfx.h"

void ped_entity_init(ped_entity_t* ent)
{
    
}

void ped_entity_tick(ped_entity_t* ent)
{

}

void ped_entity_draw(ped_entity_t* ent)
{
    mat4 ident = MAT4_IDENTITY;
    mat4 modelmat = MAT4_IDENTITY;

    vec3f future_pos = vec3f_sum(ent->pos, ent->velocity);
    vec3f future_rotation = vec3f_sum(ent->rotation, ent->rotation_velocity);
    float k1 = 1 - sys.interpolation;
    float k2 = sys.interpolation;

    vec3f visual_pos = vec3f_sum(vec3f_prod(ent->pos, k1), vec3f_prod(future_pos, k2));
    vec3f visual_rotation = vec3f_sum(vec3f_prod(ent->rotation, k1), vec3f_prod(future_rotation, k2));

    mat4_translate(&modelmat, visual_pos);
    mat4_rotate_z(&modelmat, visual_rotation.z);
    mat4_translate(&modelmat, VEC3F(-12, -30, 0));
    mat4_scale(&modelmat, VEC3F(24, 60, 1));

    glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
    glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, ident.v);

    glBindTexture(GL_TEXTURE_2D, ent->pedtype->texture);
    glBindVertexArray(shader.quad_vao);
    glDrawArrays(GL_QUADS, 0, 4);
}

void ped_entity_deinit(ped_entity_t* ent)
{

}

void ped_entity_set_type(ped_entity_t* ped, ped_type_t* type)
{
    ped->pedtype = type;
}

FINALIZE_ENTITY_TYPE(ped_entity, ped_entity_init, ped_entity_deinit, ped_entity_draw, ped_entity_tick);
