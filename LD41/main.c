#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include "game.h"
#include "gamestate.h"
#include "menu_main.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

unsigned int
main_screen_width(void)
{
    return SCREEN_WIDTH;
}

unsigned int
main_screen_height(void)
{
    return SCREEN_HEIGHT;
}

int main(int argc, char* argv[])
{
    SDL_Window         *window;
    SDL_Renderer       *renderer;
    SDL_Event           e;
    bool                run = true;
    float               last_ticks = 0.0f;
    float               ticks;
    float               frametime;
    gamestate_mgr_type  gamestate_mgr = { 0 };

    srand(time(NULL));

    (void)SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    (void)IMG_Init(IMG_INIT_PNG);
    (void)TTF_Init();
    (void)Mix_Init(MIX_INIT_OGG);
    (void)Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    window = SDL_CreateWindow("LD41",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, 0);
    (void)SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    gamestate_push(&gamestate_mgr, game_init(renderer));
    gamestate_push(&gamestate_mgr, menu_main_init(renderer));

    while (run) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                run = false;
                break;

            default:
                gamestate_event(&e, &gamestate_mgr);
                break;
            }
        }

        // TODO: quit if no gamestates active?

        ticks = (float)SDL_GetTicks() / 1000.0f;
        frametime = ticks - last_ticks;
        last_ticks = ticks;

        gamestate_update(frametime, &gamestate_mgr);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        gamestate_draw(renderer, &gamestate_mgr);
        SDL_RenderPresent(renderer);
    }

    // TODO: gamestate cleanup

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
