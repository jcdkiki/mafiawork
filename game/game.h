#ifndef DOC_GAME_H
#define DOC_GAME_H

#include "def.h"
#include "entity.h"
#include "gfx.h"

void game_init();
void game_tick();
void game_key_up(int key);
void game_key_down(int key);
void game_deinit();
void game_draw();

#endif