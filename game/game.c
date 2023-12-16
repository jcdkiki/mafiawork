#include "def.h"

#include "game.h"
#include "sys.h"
#include "exmath.h"
#include "entities/car_entity.h"
#include "entities/ped_entity.h"
#include "gfx.h" 
#include "gfx/gui.h"
#include <stdio.h>
#include <time.h>

#include "utils.h"
#include "rwstream.h"
#include "audio.h"

typedef struct player {
    ped_entity_t* ped;
} player_t;

font_t font;

car_model_t car_model;
car_entity_t *car;

ped_type_t pedtype;
ped_entity_t *ped;

player_t player;

uint16_t* mapdata;
uint32_t map_width, map_height;

uint32_t tileset_width, tileset_height;
textureid_t tileset_tx;
textureid_t crosshair_tx;

struct {
    gui_window_t win;
    gui_label_t label_hey;
    gui_label_t label_bye;

    gui_static_container_t sc;
    gui_element_t* sc_content[2];
} sample_gui;

void game_init()
{
    car_model.tx = gfx_cache_texture("textures/car.png", TEXTURE_NEAREST_FILTER);
    car_model.engine_force_max = 50.f;
    audio_sample_create_from_wavfile(&car_model.engine_sound_sample, "sounds/car4f.wav");

    audio_sample_create_from_wavfile(&car_noises.tire_screech, "sounds/screech.wav");
    
    car = (car_entity_t*)entity_create(&car_entity_vtable);
    car_entity_set_model(car, &car_model);

    pedtype.texture = gfx_cache_texture("textures/ped.png", TEXTURE_NEAREST_FILTER);
    ped = (ped_entity_t*)entity_create(&ped_entity_vtable);
    ped->pos = VEC3F(100.f, 100.f, 0.f);
    ped_entity_set_type(ped, &pedtype);

    player.ped = ped;

    crosshair_tx = gfx_cache_texture("textures/aim.png", TEXTURE_NEAREST_FILTER);

    tileset_width = 4;
    tileset_height = 115;
    tileset_tx = gfx_cache_texture("textures/tiles.png", TEXTURE_NEAREST_FILTER);

    map_width = 256;
    map_height = 256;
    mapdata = sys_malloc(sizeof(*mapdata) * map_width * map_height);

    for (uint32_t i = 0; i < map_width * map_height; i++)
        mapdata[i] = rand() % (tileset_width*tileset_height);

    *list_emplace_front(&entlist, base_entity_t*) = (base_entity_t*)car;
    *list_emplace_front(&entlist, base_entity_t*) = (base_entity_t*)ped;

    font_create(&font, gfx_cache_texture("textures/font.png", TEXTURE_NEAREST_FILTER), ' ', 20, 5, VEC3F(10, 24, 0));

    sample_gui.sc_content[0] = &sample_gui.label_hey;
    sample_gui.sc_content[1] = &sample_gui.label_bye;

    gui_init_window(&sample_gui.win, &sample_gui.sc, 0);
    gui_init_static_container(&sample_gui.sc, 2, sample_gui.sc_content, 0);
    gui_init_label(&sample_gui.label_hey, &font, "Hello bro", 0);
    gui_init_label(&sample_gui.label_bye, &font, "bye-bye", 0);
    
    gui_calculate_dimensions(&sample_gui.win);
}

void game_tick()
{
    for (listnode_t* ent = entlist.begin; ent; ent = ent->next)
        entity_tick(LISTNODE_DATA(ent, base_entity_t*));
}

void game_key_up(int key)
{
}

void game_key_down(int key)
{
    if (key == KEY_ESCAPE) sys_close_window();
}

void game_deinit()
{
    for (listnode_t* node = entlist.begin; node; node = node->next) {
        entity_destroy(LISTNODE_DATA(node, base_entity_t*));
    }

    audio_sample_destroy(&car_model.engine_sound_sample);
    audio_sample_destroy(&car_noises.tire_screech);

    gui_destroy_elements(&sample_gui.win);

    list_destroy(&entlist);
}

float interpolate(float a, float b, float i)
{
    return b*i + a*(1.f - i);
}

void game_draw()
{
    mat4 identity = MAT4_IDENTITY;
    mat4 view;
    
    /* view calculation */
    {
        base_entity_t* chase_entity = (base_entity_t*)car;
        float scale = 1.f;
        vec3f interpolated_pos = vec3f_neg(vec3f_sum(chase_entity->pos, vec3f_prod(chase_entity->velocity, sys.interpolation)));
        mat4_translation(&view, VEC3F(sys.width / 2.f, sys.height / 2.f, 0.f));
        mat4_scale(&view, VEC3F(scale, scale, 1));
        mat4_translate(&view, interpolated_pos);
    }

    glDepthFunc(GL_ALWAYS);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    gfx_setup_xy_screen_matrices();
    glUniformMatrix4fv(shader.view_mat_location, 1, GL_TRUE, view.v);

    
    glBindTexture(GL_TEXTURE_2D, tileset_tx);
    glBindVertexArray(shader.quad_vao);
    for (uint32_t x = 0; x < 64; x++) {
        for (uint32_t y = 0; y < 64; y++) {
            uint16_t tile = mapdata[x + y*map_width];
            uint32_t tile_x = tile % tileset_width;
            uint32_t tile_y = tile / tileset_width;

            mat4 modelmat;
            mat4 texmat;

            mat4_translation(&modelmat, VEC3F(x * 64.f, y * 64.f, 0.f));
            mat4_scale(&modelmat, VEC3F(64.f, 64.f, 1.f));
            mat4_translation(&texmat, VEC3F((float)tile_x / tileset_width, (float)tile_y / tileset_height, 0.f));
            mat4_scale(&texmat, VEC3F(1.f / tileset_width, 1.f / tileset_height, 1.f));

            glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
            glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, texmat.v);
            
            glDrawArrays(GL_QUADS, 0, 4);
        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (listnode_t* ent = entlist.begin; ent; ent = ent->next)
        entity_draw(LISTNODE_DATA(ent, base_entity_t*));

    glBindTexture(GL_TEXTURE_2D, shader.white_texture);
    glBindVertexArray(shader.quad_vao);

    /* 
     * GUI
     */
    glUniformMatrix4fv(shader.view_mat_location, 1, GL_TRUE, identity.v);

    gfx_draw_2d_texture(crosshair_tx, sys.mouse.x - 16.f, sys.mouse.y - 16.f, 32.f, 32.f);

    gui_context_t guictx;
    guictx.pos = VEC2F(0.f, 0.f);

    gui_draw_element(&sample_gui.win, &guictx);

    str8 str;
    str8_create_by_printf(&str,
        "speed: %d (units per tick)\n"
        "engine_force: %.2f\n"
        ,
        (int)vec3f_len(car->velocity),
        car->engine_force
    );

    //gfx_draw_text(str.data, &font, VEC3F(5.f, 5.f, 0.f), VEC3F(1.f, 1.f, 0.f));
    str8_destroy(&str);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}