#include <assert.h>
#include <SDL.h>
#include "gamestate.h"

#define GAMESTATE_TOP(_mgr) (&(_mgr)->gamestate_stack[(_mgr)->gamestate_count - 1])

void
gamestate_push(gamestate_mgr_type *mgr, gamestate_type state)
{
    assert(mgr->gamestate_count < MAX_GAMESTATES);
    mgr->gamestate_stack[mgr->gamestate_count++] = state;
}


void
gamestate_replace(gamestate_mgr_type *mgr, gamestate_type state)
{
    gamestate_type *top = GAMESTATE_TOP(mgr);
    top->cleanup_cb(top->ctx);
    *top = state;
}


void
gamestate_replace_all(gamestate_mgr_type *mgr, gamestate_type state)
{
    for (size_t i = 0; i < mgr->gamestate_count; i++) {
        gamestate_type *state = &mgr->gamestate_stack[i];
        state->cleanup_cb(state->ctx);
    }
    mgr->gamestate_stack[0] = state;
    mgr->gamestate_count = 1;
}


void
gamestate_pop(gamestate_mgr_type *mgr)
{
    gamestate_type *state = GAMESTATE_TOP(mgr);
    state->cleanup_cb(state->ctx);
    mgr->gamestate_count--;
}


void
gamestate_event(SDL_Event *e, gamestate_mgr_type *mgr)
{
    gamestate_type *state = GAMESTATE_TOP(mgr);
    state->event_cb(mgr, e, state->ctx);
}


void
gamestate_update(float frametime, gamestate_mgr_type *mgr)
{
    gamestate_type *state = GAMESTATE_TOP(mgr);
    state->update_cb(mgr, frametime, state->ctx);
}


void
gamestate_draw(SDL_Renderer *renderer, const gamestate_mgr_type *mgr)
{
    const gamestate_type *state = GAMESTATE_TOP(mgr);
    gamestate_type *under;

    // Note this only supports a single layer of drawing under.
    if ((state->flags & GAMESTATE_FLAG_DRAW_UNDER) != 0 && mgr->gamestate_count > 1) {
        under = &mgr->gamestate_stack[mgr->gamestate_count - 2];
        under->draw_cb(renderer, under->ctx);
    }

    state->draw_cb(renderer, state->ctx);
}