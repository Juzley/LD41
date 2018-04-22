#include <SDL.h>

#include "font.h"
#include "gamestate.h"
#include "main.h"
#include "tutorial.h"
#include "utils.h"

#define BIG_FONT_SIZE 128
#define SMALL_FONT_SIZE 64
#define MINI_FONT_SIZE 24
#define PAUSE_TIME 0.5f

typedef struct menu_main_info {
    mapped_font_handle big_font;
    mapped_font_handle small_font;
    mapped_font_handle mini_font;
    SDL_Renderer *renderer;
    float time;
} menu_main_info_type;

static void
menu_main_update(gamestate_mgr_handle mgr,
                 float frametime,
                 menu_main_info_type *menu)
{
    menu->time += frametime;
}

static void
menu_main_draw(SDL_Renderer *renderer,
               const menu_main_info_type *menu)
{
    int y = SMALL_FONT_SIZE;
    int center = main_screen_width() / 2;

    draw_overlay(renderer, main_screen_width(), main_screen_height());

    mapped_font_draw_ex(renderer, menu->big_font, center, y, 0, 0, 0, color_white, ALIGN_CENTER, "LD41");
    y += BIG_FONT_SIZE + SMALL_FONT_SIZE;

    mapped_font_draw_ex(renderer, menu->small_font, center, y, 0, 0, 0, color_white, ALIGN_CENTER, "Press T for tutorial");
    y += SMALL_FONT_SIZE * 2;
    mapped_font_draw_ex(renderer, menu->small_font, center, y, 0, 0, 0, color_white, ALIGN_CENTER, "Press any other key to start");

    mapped_font_draw_ex(renderer, menu->mini_font, center, main_screen_height() - MINI_FONT_SIZE * 2, 0, 0, 0,
                        color_white, ALIGN_CENTER, "Game by Juzley, Assets from kenney.nl");
}

static void
menu_main_event(gamestate_mgr_handle mgr,
                SDL_Event *e,
                menu_main_info_type *menu)
{
    switch (e->type) {
    case SDL_KEYDOWN:
        if (e->key.keysym.sym == SDLK_t) {
            gamestate_push(mgr, tutorial_init(menu->renderer));
            break;
        }
    // Deliberate fallthrough
    case SDL_MOUSEBUTTONUP:
        if (menu->time > PAUSE_TIME) {
            gamestate_pop(mgr);
        }

        break;
    }
}

static void
menu_main_cleanup(menu_main_info_type *menu)
{
    mapped_font_destroy(menu->mini_font);
    mapped_font_destroy(menu->small_font);
    mapped_font_destroy(menu->big_font);
    free(menu);
}

gamestate_type
menu_main_init(SDL_Renderer *renderer)
{
    gamestate_type gamestate;
    menu_main_info_type *menu;

    menu = calloc(1, sizeof(*menu));
    menu->renderer = renderer;
    menu->big_font = mapped_font_create(renderer, "media/fonts/hud.ttf", BIG_FONT_SIZE);
    menu->small_font = mapped_font_create(renderer, "media/fonts/hud.ttf", SMALL_FONT_SIZE);
    menu->mini_font = mapped_font_create(renderer, "media/fonts/hud.ttf", MINI_FONT_SIZE);

    gamestate.update_cb = (gamestate_update_fn_type)&menu_main_update;
    gamestate.draw_cb = (gamestate_draw_fn_type)&menu_main_draw;
    gamestate.event_cb = (gamestate_event_fn_type)&menu_main_event;
    gamestate.cleanup_cb = (gamestate_cleanup_fn_type)&menu_main_cleanup;
    gamestate.flags = GAMESTATE_FLAG_DRAW_UNDER;
    gamestate.ctx = menu;

    return gamestate;
}