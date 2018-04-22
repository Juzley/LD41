#ifndef __GAME_OVER_H__
#define __GAME_OVER_H__

#include <SDL.h>

#include "font.h"
#include "game.h"
#include "gamestate.h"
#include "main.h"
#include "menu_main.h"
#include "utils.h"

#define BIG_FONT_SIZE 128
#define SMALL_FONT_SIZE 64
#define PAUSE_TIME 1.0f

typedef struct gameover_info {
    mapped_font_handle big_font;
    mapped_font_handle small_font;
    SDL_Renderer *renderer; // Hang on to this for creating new gamestates.
    float time;
} gameover_info_type;

static inline void
gameover_update(gamestate_mgr_handle mgr,
                float frametime,
                gameover_info_type *gameover)
{
    gameover->time += frametime;
}

static inline void
gameover_draw(SDL_Renderer *renderer,
              const gameover_info_type *gameover)
{
    int center = main_screen_width() / 2;

    draw_overlay(renderer, main_screen_width(), main_screen_height());

    mapped_font_draw_ex(renderer, gameover->big_font, center, SMALL_FONT_SIZE, 0, 0, 0, color_white, ALIGN_CENTER, "Game Over!");
 
    if (gameover->time > PAUSE_TIME) {
        mapped_font_draw_ex(renderer, gameover->small_font, center, main_screen_height() - SMALL_FONT_SIZE * 2, 0, 0, 0,
                            color_white, ALIGN_CENTER, "Press any key to continue");
    }
}

static inline void
gameover_event(gamestate_mgr_handle mgr,
               SDL_Event *e,
               gameover_info_type *gameover)
{
    gamestate_type new_game;
    gamestate_type new_menu;


    switch (e->type) {
    case SDL_KEYDOWN:
    case SDL_MOUSEBUTTONUP:
        if (gameover->time > PAUSE_TIME) {
            // Calling gamestate_replace_all will free this gamestate,
            // so do anything that needs access to it up front.
            new_game = game_init(gameover->renderer);
            new_menu = menu_main_init(gameover->renderer);

            gamestate_replace_all(mgr, new_game);
            gamestate_push(mgr, new_menu);
            gameover = NULL;
        }
    }
}

static void
gameover_cleanup(gameover_info_type *gameover)
{
    mapped_font_destroy(gameover->small_font);
    mapped_font_destroy(gameover->big_font);
    free(gameover);
}

static inline gamestate_type
gameover_init(SDL_Renderer *renderer) {
    gamestate_type gamestate;
    gameover_info_type *gameover;

    gameover = calloc(1, sizeof(*gameover));
    gameover->renderer = renderer;
    gameover->big_font = mapped_font_create(renderer, "media/fonts/hud.ttf", BIG_FONT_SIZE);
    gameover->small_font = mapped_font_create(renderer, "media/fonts/hud.ttf", SMALL_FONT_SIZE);
    
    gamestate.update_cb = (gamestate_update_fn_type)&gameover_update;
    gamestate.draw_cb = (gamestate_draw_fn_type)&gameover_draw;
    gamestate.event_cb = (gamestate_event_fn_type)&gameover_event;
    gamestate.cleanup_cb = (gamestate_cleanup_fn_type)&gameover_cleanup;
    gamestate.flags = GAMESTATE_FLAG_DRAW_UNDER;
    gamestate.ctx = gameover;

    return gamestate;
}

#endif /* __GAME_OVER_H__ */
