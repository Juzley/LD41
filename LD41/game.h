#ifndef __GAME_H__
#define __GAME_H__

#include <SDL.h>
#include "gamestate.h"

gamestate_type game_init(SDL_Renderer *renderer);

#endif __GAME_H__