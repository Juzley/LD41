#ifndef __TUTORIAL_H__
#define __TUTORIAL_H__

#include <SDL.h>

#include "font.h"
#include "gamestate.h"
#include "main.h"
#include "utils.h"

#define NUM_SCREENS 4

typedef struct tutorial_info {
    SDL_Texture *screens[NUM_SCREENS];
    size_t cur_screen;
    float time;
} tutorial_info_type;

static void
tutorial_update(gamestate_mgr_handle mgr,
                float frametime,
                tutorial_info_type *tutorial)
{
    tutorial->time += frametime;
}

static void
tutorial_draw(SDL_Renderer *renderer,
              const tutorial_info_type *tutorial)
{
    SDL_Rect rect;

    SDL_Texture *screen = tutorial->screens[tutorial->cur_screen];
        
    SDL_QueryTexture(screen, NULL, NULL, &rect.w, &rect.h);
    rect.x = (main_screen_width() - rect.w) / 2;
    rect.y = (main_screen_height() - rect.h) / 2;
    SDL_RenderCopy(renderer, screen, NULL, &rect);
}

static void
tutorial_event(gamestate_mgr_handle mgr,
               SDL_Event *e,
               tutorial_info_type *tutorial)
{
    switch (e->type) {
    case SDL_KEYDOWN:
    case SDL_MOUSEBUTTONUP:
        tutorial->cur_screen++;
        if (tutorial->cur_screen >= NUM_SCREENS) {
            gamestate_pop(mgr);
        }
        break;
    }
}

static void
tutorial_cleanup(tutorial_info_type *tutorial)
{
    for (size_t i = 0; i < NUM_SCREENS; i++) {
        free_texture(tutorial->screens[i]);
    }
    
    free(tutorial);
}

gamestate_type
tutorial_init(SDL_Renderer *renderer)
{
    gamestate_type gamestate;
    tutorial_info_type *tutorial;

    tutorial = calloc(1, sizeof(*tutorial));
    tutorial->screens[0] = load_texture("media/tutorial/tut1.png", renderer);
    tutorial->screens[1] = load_texture("media/tutorial/tut2.png", renderer);
    tutorial->screens[2] = load_texture("media/tutorial/tut3.png", renderer);
    tutorial->screens[3] = load_texture("media/tutorial/tut4.png", renderer);
    
    gamestate.update_cb = (gamestate_update_fn_type)&tutorial_update;
    gamestate.draw_cb = (gamestate_draw_fn_type)&tutorial_draw;
    gamestate.event_cb = (gamestate_event_fn_type)&tutorial_event;
    gamestate.cleanup_cb = (gamestate_cleanup_fn_type)&tutorial_cleanup;
    gamestate.flags = GAMESTATE_FLAG_DEFAULT;
    gamestate.ctx = tutorial;

    return gamestate;
}

#endif /* __TUTORIAL_H__ */
