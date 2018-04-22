#ifndef __GAMESTATE_H__
#define __GAMESTATE_H__


#include <SDL.h>


#define MAX_GAMESTATES 16

typedef struct gamestate_mgr *gamestate_mgr_handle;

typedef void(*gamestate_event_fn_type)(gamestate_mgr_handle mgr,
                                       SDL_Event *e,
                                       void      *ctx);
typedef void(*gamestate_update_fn_type)(gamestate_mgr_handle mgr,
                                        float frametime,
                                        void *ctx);
typedef void(*gamestate_draw_fn_type)(SDL_Renderer *renderer,
                                      void         *ctx);
typedef void(*gamestate_cleanup_fn_type)(void *ctx);

typedef uint8_t gamestate_flag_type;
#define GAMESTATE_FLAG_DEFAULT    0x00
#define GAMESTATE_FLAG_DRAW_UNDER 0x01

typedef struct gamestate {
    gamestate_event_fn_type   event_cb;
    gamestate_update_fn_type  update_cb;
    gamestate_draw_fn_type    draw_cb;
    gamestate_cleanup_fn_type cleanup_cb;
    void                     *ctx;
    gamestate_flag_type       flags;
} gamestate_type;

typedef struct gamestate_mgr {
    gamestate_type gamestate_stack[MAX_GAMESTATES];
    size_t         gamestate_count;
} gamestate_mgr_type;

void gamestate_push(gamestate_mgr_type *mgr, gamestate_type state);
void gamestate_replace(gamestate_mgr_type *mgr, gamestate_type state);
void gamestate_replace_all(gamestate_mgr_type *mgr, gamestate_type state);
void gamestate_pop(gamestate_mgr_type *mgr);
void gamestate_event(SDL_Event *e, gamestate_mgr_type *mgr);
void gamestate_update(float frametime, gamestate_mgr_type *mgr);
void gamestate_draw(SDL_Renderer *renderer, const gamestate_mgr_type *mgr);


#endif /* __GAMESTATE_H__ */
