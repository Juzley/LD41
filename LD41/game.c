#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "font.h"
#include "gameover.h"
#include "gamestate.h"
#include "utils.h"


typedef enum tile {
    TILE_SHIP,
    TILE_LASER,
    TILE_ENEMY_LASER,
    TILE_ENEMY,
    TILE_ASTEROID_1,
    TILE_ASTEROID_2,
    TILE_ASTEROID_3,
    TILE_BOMB,
    TILE_EMPTY,
    TILE_COUNT = TILE_EMPTY,
} tile_type;

tile_type tile_weights[] = {
    TILE_SHIP, TILE_SHIP, TILE_SHIP, TILE_SHIP,
    TILE_ENEMY, TILE_ENEMY, TILE_ENEMY, TILE_ENEMY,
    TILE_LASER, TILE_LASER, TILE_LASER, TILE_LASER, TILE_LASER, TILE_LASER,
    TILE_ENEMY_LASER, TILE_ENEMY_LASER, TILE_ENEMY_LASER,
    TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1, TILE_ASTEROID_1,
    TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2, TILE_ASTEROID_2,
    TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3, TILE_ASTEROID_3,
    TILE_BOMB
};

typedef struct coord {
    size_t x;
    size_t y;
} coord_type;

#define BOARD_WIDTH 8
#define BOARD_HEIGHT 8

// TODO: Resolution magic numbers
#define TILE_WIDTH (800 / BOARD_WIDTH)
#define TILE_HEIGHT (800 / BOARD_HEIGHT)

#define HUD_PADDING 10
#define HUD_WIDTH (1280 - 800 - HUD_PADDING * 2)
#define HUD_START_X (TILE_WIDTH * BOARD_WIDTH + HUD_PADDING)
#define HUD_BAR_HEIGHT 48

#define HUD_TEXT_HEIGHT 32
#define HUD_TEXT_LARGE_HEIGHT 64

#define MAX_ENERGY 100
#define MOVE_ENERGY 12
#define KILL_ENERGY 25
#define DIE_ENERGY 35
#define MATCH_ENERGY 3
#define TICK_ENERGY 1

#define DROP_TIME 0.1f
#define SWAP_TIME 0.1f
#define ENERGY_TICK_TIME 5.0f

typedef enum {
    GAME_STATE_IDLE,
    GAME_STATE_DROPPING,
    GAME_STATE_SWAPPING,
} game_state_type;

typedef struct game_info {
    SDL_Renderer   *renderer;
    tile_type       tiles[BOARD_HEIGHT][BOARD_WIDTH];
    tile_type       next_row[BOARD_WIDTH];
    float           game_time;
    game_state_type game_state;
    float           update_time;
    float           tick_time;
    coord_type      mouse_down_coords;
    coord_type      swap_a;
    coord_type      swap_b;
    uint8_t         energy;
    uint32_t        score;
    uint8_t         chain;

    // Fonts
    mapped_font_handle hud_font;
    mapped_font_handle hud_font_large;

    // Textures
    SDL_Texture *ship_texture;
    SDL_Texture *enemy_texture;
    SDL_Texture *asteroid_1_texture;
    SDL_Texture *asteroid_2_texture;
    SDL_Texture *asteroid_3_texture;
    SDL_Texture *bomb_texture;
    SDL_Texture *laser_texture;
    SDL_Texture *enemy_laser_texture;
    SDL_Texture *energy_bar_left;
    SDL_Texture *energy_bar_mid;
    SDL_Texture *energy_bar_right;
    SDL_Texture *energy_bar_back_left;
    SDL_Texture *energy_bar_back_mid;
    SDL_Texture *energy_bar_back_right;
    SDL_Texture *energy_bar_yellow_left;
    SDL_Texture *energy_bar_yellow_mid;
    SDL_Texture *energy_bar_yellow_right;
    SDL_Texture *energy_bar_red_left;
    SDL_Texture *energy_bar_red_mid;
    SDL_Texture *energy_bar_red_right;

    // Sounds
    Mix_Chunk *swap_sound;
    Mix_Chunk *shoot_sound;
    Mix_Chunk *enemy_shoot_sound;
    Mix_Chunk *match_sound;
} game_info_type;

game_lose_energy(game_info_type *game,
                 uint8_t         damage)
{
    if (game->energy > damage) {
        game->energy -= damage;
    } else {
        game->energy = 0;
    }
}

static void
game_draw_tile(SDL_Renderer *renderer,
               size_t x,
               size_t y,
               uint8_t r,
               uint8_t g,
               uint8_t b,
               int y_offset)
{
    SDL_Rect rect;

    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    rect.x = x * TILE_WIDTH;
    rect.y = y * TILE_HEIGHT + y_offset;
    rect.w = TILE_WIDTH;
    rect.h = TILE_HEIGHT;
    SDL_RenderFillRect(renderer, &rect);
}

static void
game_draw_tile_textured(SDL_Renderer *renderer,
                        SDL_Texture *texture,
                        size_t x,
                        size_t y,
                        int x_offset,
                        int y_offset)
{
    SDL_Rect rect;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    rect.x = x * TILE_WIDTH + x_offset;
    rect.y = y * TILE_HEIGHT + y_offset;
    rect.w = TILE_WIDTH;
    rect.h = TILE_HEIGHT;
    SDL_RenderCopy(renderer, texture, NULL, &rect);
}

static coord_type
game_window_coords_to_tile (int32_t x, int32_t y)
{
    coord_type res;
    res.x = x / TILE_WIDTH;
    res.y = y / TILE_HEIGHT;
    return res;
}

static void
game_mark_erased(bool erase_tiles[BOARD_HEIGHT][BOARD_WIDTH],
                 size_t start_x,
                 size_t start_y,
                 size_t x_inc,
                 size_t y_inc,
                 size_t distance) {
    for (size_t i = 0; i <= distance; i++) {
        erase_tiles[start_x + i * x_inc][start_y + i * y_inc] = true;
    }
}

static void
game_mark_erased_square(
    game_info_type *game,
    bool erase_tiles[BOARD_HEIGHT][BOARD_WIDTH],
    int mid_x,
    int mid_y,
    uint8_t *enemies_erased,
    uint8_t *ships_erased)
{
    for (int x = MAX(mid_x - 1, 0); x < BOARD_WIDTH && x <= mid_x + 1; x++) {
        for (int y = MAX(mid_y - 1, 0); y < BOARD_HEIGHT && y <= mid_y + 1; y++) {
            erase_tiles[x][y] = true;

            if (enemies_erased != NULL && game->tiles[x][y] == TILE_ENEMY) {
                *enemies_erased += 1;
            }
            if (ships_erased != NULL && game->tiles[x][y] == TILE_SHIP) {
                *ships_erased += 1;
            }
        }
    }

}

static void
game_check_shot(game_info_type *game,
                bool erase_tiles[BOARD_HEIGHT][BOARD_WIDTH],
                size_t start_x,
                size_t start_y,
                bool *updated,
                uint8_t *enemies_killed,
                uint8_t *ships_killed)
{
    size_t y;
    tile_type org_tile = game->tiles[start_x][start_y];
    tile_type prev_tile = org_tile;
    tile_type cur_tile;


    if (org_tile != TILE_ENEMY && org_tile != TILE_BOMB) {
        // This isn't a shootable thing.
        return;
    }

    if (start_y >= BOARD_HEIGHT - 2) {
        // Not enough space on the board for shooting.
        return;
    }

    for (y = start_y + 1; y < BOARD_HEIGHT; y++) {
        cur_tile = game->tiles[start_x][y];
        if (org_tile == TILE_ENEMY && cur_tile == TILE_SHIP && prev_tile == TILE_LASER) {
            // Player shoots enemy.
            game_mark_erased(erase_tiles, start_x, start_y, 0, 1, y - start_y - 1);
            *enemies_killed += 1;
            *updated = true;
        } else if (org_tile == TILE_ENEMY && cur_tile == TILE_SHIP && prev_tile == TILE_ENEMY_LASER) {
            // Enemy shoots player.
            game_mark_erased(erase_tiles, start_x, start_y + 1, 0, 1, y - start_y);
            *ships_killed += 1;
            *updated = true;
        } else if (org_tile == TILE_ENEMY && cur_tile == TILE_BOMB && prev_tile == TILE_ENEMY_LASER) {
            // Enemy shoots bomb.
            game_mark_erased(erase_tiles, start_x, start_y + 1, 0, 1, y - start_y);
            // We don't count an enemy killing an enemy to the score.
            game_mark_erased_square(game, erase_tiles, start_x, y, NULL, ships_killed);
            *updated = true;
        } else if (org_tile == TILE_BOMB && cur_tile == TILE_SHIP && prev_tile == TILE_LASER) {
            // Player shoots bomb.
            game_mark_erased(erase_tiles, start_x, start_y, 0, 1, y - start_y);
            game_mark_erased_square(game, erase_tiles, start_x, start_y, enemies_killed, ships_killed);
            *updated = true;
        } else if ((cur_tile != TILE_LASER || prev_tile == TILE_ENEMY_LASER) &&
            (cur_tile != TILE_ENEMY_LASER || prev_tile == TILE_LASER)) {
            // If this isn't valid run of laser fire, stop searching.
            break;
        }

        prev_tile = cur_tile;
    }
}

static void
game_check_match(game_info_type *game,
                 bool erase_tiles[BOARD_HEIGHT][BOARD_WIDTH],
                 size_t start_x,
                 size_t start_y,
                 size_t x_inc,
                 size_t y_inc,
                 bool *updated)
{
    tile_type orig = game->tiles[start_x][start_y];
    tile_type prev = orig;
    tile_type cur;
    size_t x;
    size_t y;
    size_t distance = 0;

    for (x = start_x + x_inc, y = start_y + y_inc; x < BOARD_WIDTH && y < BOARD_HEIGHT; x += x_inc, y += y_inc) {
        cur = game->tiles[x][y];
        distance = x - start_x + y - start_y;

        if (cur != prev) {
            if (distance > 2) {
                game_mark_erased(erase_tiles, start_x, start_y, x_inc, y_inc, distance - 1);
                game->energy = MIN(MAX_ENERGY, game->energy + (distance - 1) * MATCH_ENERGY);
                *updated = true;
            }
            break;
        }

        prev = cur;
    }

    // If we stopped because we hit the end of the board, check if we had found a match before stopping.
    if ((x == BOARD_WIDTH || y == BOARD_HEIGHT) && prev == orig && distance > 1) {
        game_mark_erased(erase_tiles, start_x, start_y, x_inc, y_inc, distance);
        game->energy = MIN(MAX_ENERGY, game->energy + distance * MATCH_ENERGY);
        *updated = true;
    }
}

static void
game_check_board(game_info_type *game,
                 bool play_sounds)
{
    bool erase_tiles[BOARD_HEIGHT][BOARD_WIDTH] = { 0 };
    size_t x;
    size_t y;
    bool updated;
    bool found_match = false;
    uint8_t enemies_killed = 0;
    uint8_t ships_killed = 0;
    
    // Check for shots landing.
    updated = false;
    for (x = 0; x < BOARD_WIDTH; x++) {
        for (y = 0; y < BOARD_HEIGHT; y++) {
            game_check_shot(game, erase_tiles, x, y, &updated, &enemies_killed, &ships_killed);
        }
    }

    if (play_sounds && enemies_killed > 0) {
        Mix_PlayChannel(-1, game->shoot_sound, 0);
    }
    if (play_sounds && ships_killed > 0) {
        Mix_PlayChannel(-1, game->enemy_shoot_sound, 0);
    }

    // TODO: More points/energy for kills from further away?
    game->score += (enemies_killed * enemies_killed) * game->chain * 100;
    game->energy += enemies_killed * KILL_ENERGY;
    game_lose_energy(game, ships_killed * DIE_ENERGY);
    game->energy = MIN(MAX_ENERGY, game->energy);

    // Check for matching runs.
    for (x = 0; x < BOARD_WIDTH; x++) {
        for (y = 0; y < BOARD_HEIGHT; y++) {
            game_check_match(game, erase_tiles, x, y, 1, 0, &found_match);
            game_check_match(game, erase_tiles, x, y, 0, 1, &found_match);
        }
    }
    if (play_sounds && found_match) {
        Mix_PlayChannel(-1, game->match_sound, 0);
    }
    updated = updated || found_match;

    // Mark-and-sweep the tiles so that if there are multiple matches/shots involving the same tiles
    // we get them all.
    for (x = 0; x < BOARD_WIDTH; x++) {
        for (y = 0; y < BOARD_HEIGHT; y++) {
            if (erase_tiles[x][y]) {
                game->tiles[x][y] = TILE_EMPTY;
            }
        }
    }

    if (updated) {
        game->game_state = GAME_STATE_DROPPING;
        game->update_time = game->game_time;
    }
}

static void
game_swap_tiles(game_info_type *game, coord_type a, coord_type b)
{
    game->swap_a = a;
    game->swap_b = b;
    game->game_state = GAME_STATE_SWAPPING;
    game->update_time = game->game_time;

    Mix_PlayChannel(-1, game->swap_sound, 0);
}

static tile_type
game_random_tile(void)
{
    unsigned int index = random_range(0, sizeof(tile_weights) / sizeof(*tile_weights) - 1);
    return tile_weights[index];
}

static void
game_update(gamestate_mgr_handle mgr,
            float frametime,
            game_info_type *game)
{
    size_t x;
    int y;
    bool dropping;
    bool finished;

    game->game_time += frametime;

    if (game->game_time > game->tick_time + ENERGY_TICK_TIME) {
        game_lose_energy(game, TICK_ENERGY);
        game->tick_time = game->game_time;
    }

    if (game->energy == 0) {
        gamestate_push(mgr, gameover_init(game->renderer));
    }

    if (game->game_state == GAME_STATE_SWAPPING && game->game_time > game->update_time + SWAP_TIME) {
        tile_type tmp = game->tiles[game->swap_a.x][game->swap_a.y];
        game->tiles[game->swap_a.x][game->swap_a.y] = game->tiles[game->swap_b.x][game->swap_b.y];
        game->tiles[game->swap_b.x][game->swap_b.y] = tmp;
        game->game_state = GAME_STATE_IDLE;
        game_check_board(game, true);
        
        // Lose the energy for moving after checking the board, so that if the player has less
        // energy than it takes to move, they can avoid dying if they make a move that gains
        // energy.
        game_lose_energy(game, MOVE_ENERGY);
    }
    
    if (game->game_state == GAME_STATE_DROPPING && game->game_time > game->update_time + DROP_TIME) {
        // Move tiles down.
        for (x = 0; x < BOARD_WIDTH; x++) {
            dropping = false;
            for (y = BOARD_HEIGHT - 1; y >= 0; y--) {
                if (game->tiles[x][y] == TILE_EMPTY) {
                    dropping = true;
                }

                if (dropping) {
                    if (y == 0) {
                        game->tiles[x][y] = game->next_row[x];
                    } else {
                        game->tiles[x][y] = game->tiles[x][y - 1];
                    }
                }
            }
        }

        // Generate a new next row.
        for (x = 0; x < BOARD_WIDTH; x++) {
            game->next_row[x] = game_random_tile();
        }

        // Check if there is more movement to be done.
        finished = true;
        for (x = 0; finished && x < BOARD_WIDTH; x++) {
            for (y = 0; finished && y < BOARD_HEIGHT; y++) {
                if (game->tiles[x][y] == TILE_EMPTY) {
                    game->update_time = game->game_time;
                    finished = false;
                }
            }
        }

        // If we've finished moving, check if there are new matches.
        if (finished) {
            game->game_state = GAME_STATE_IDLE;
            
            game_check_board(game, true);
            
            // If we found new matches, increase the chain, otherwise reset it.
            if (game->game_state == GAME_STATE_DROPPING) {
                game->chain += 1;
            } else {
                game->chain = 1;
            }
        }
    }
   
}

static void
game_draw_hud_bar(SDL_Renderer *renderer,
                  int y,
                  int height,
                  int start,
                  int end,
                  SDL_Texture *left_texture,
                  SDL_Texture *mid_texture,
                  SDL_Texture *right_texture)
{
    SDL_Rect rect;
    int i;

    rect.x = start;
    rect.y = y;
    rect.w = 8;
    rect.h = height;
    SDL_RenderCopy(renderer, left_texture, NULL, &rect);

    rect.x = MAX(end - 8, start + 8);
    SDL_RenderCopy(renderer, right_texture, NULL, &rect);

    const int mid_start = start + 8;
    const int mid_width = end - start - 16;
    const int mid_chunks = mid_width / 24;
    const int mid_remainder = mid_width - mid_chunks * 24;

    rect.w = 24;
    for (i = 0; i < mid_chunks; i++) {
        rect.x = mid_start + i * 24;
        SDL_RenderCopy(renderer, mid_texture, NULL, &rect);
    }

    rect.x = mid_start + i * 24;
    rect.w = mid_remainder;
    SDL_RenderCopy(renderer, mid_texture, NULL, &rect);
}

static void
game_draw_hud(SDL_Renderer *renderer,
              const game_info_type *game)
{
    float energy_ratio;
    SDL_Texture *energy_left;
    SDL_Texture *energy_mid;
    SDL_Texture *energy_right;
    int y = 6;
    int minutes;
    int seconds;

    mapped_font_draw(renderer, game->hud_font, HUD_START_X, y, "Energy");
    
    y += HUD_TEXT_HEIGHT;
    game_draw_hud_bar(renderer, y, HUD_BAR_HEIGHT, HUD_START_X, HUD_START_X + HUD_WIDTH,
                      game->energy_bar_back_left, game->energy_bar_back_mid, game->energy_bar_back_right);

    energy_ratio = (float)game->energy / (float)MAX_ENERGY;
    if (energy_ratio > 0.66f) {
        energy_left = game->energy_bar_left;
        energy_mid = game->energy_bar_mid;
        energy_right = game->energy_bar_right;
    } else if (energy_ratio > 0.33f) {
        energy_left = game->energy_bar_yellow_left;
        energy_mid = game->energy_bar_yellow_mid;
        energy_right = game->energy_bar_yellow_right;
    } else {
        energy_left = game->energy_bar_red_left;
        energy_mid = game->energy_bar_red_mid;
        energy_right = game->energy_bar_red_right;
    }

    game_draw_hud_bar(renderer, y, HUD_BAR_HEIGHT, HUD_START_X, (int)(HUD_START_X + HUD_WIDTH * energy_ratio),
                      energy_left, energy_mid, energy_right);

    y += HUD_BAR_HEIGHT + HUD_TEXT_HEIGHT;
    mapped_font_draw(renderer, game->hud_font, HUD_START_X, y, "Score");
    y += HUD_TEXT_HEIGHT;
    mapped_font_drawf_ex(renderer, game->hud_font_large, main_screen_width() - 4, y, ALIGN_RIGHT, "%u", game->score);

    y += HUD_TEXT_LARGE_HEIGHT;
    mapped_font_draw(renderer, game->hud_font, HUD_START_X, y, "Time");
    y += HUD_TEXT_HEIGHT;

    minutes = (int)game->game_time / 60;
    seconds = (int)game->game_time - minutes * 60;
    mapped_font_drawf_ex(renderer, game->hud_font_large, main_screen_width() - 4, y, ALIGN_RIGHT, "%d:%02d", minutes, seconds);

}

static void
game_draw(SDL_Renderer         *renderer,
          const game_info_type *game)
{
    tile_type tile;
    size_t x;
    int y;
    int y_offset;
    int x_offset = 0;
    int swap_distance;

    game_draw_hud(renderer, game);

    for (x = 0; x < BOARD_HEIGHT; x++) {
        y_offset = 0;
        for (y = BOARD_HEIGHT - 1; y >= -1; y--) {
            // Draw the next row dropping in if required.
            if (y < 0) {
                if (y_offset != 0) {
                    tile = game->next_row[x];
                } else {
                    break;
                }
            } else {
                tile = game->tiles[x][y];
            }

            // If we're swapping tiles, draw them moving.
            if (game->game_state == GAME_STATE_SWAPPING) {
                x_offset = 0;
                y_offset = 0;
                swap_distance = (int)(((game->game_time - game->update_time) / DROP_TIME) * TILE_HEIGHT);
                if (game->swap_a.x == x && game->swap_a.y == y) {
                    x_offset = swap_distance * (game->swap_b.x - game->swap_a.x);
                    y_offset = swap_distance * (game->swap_b.y - game->swap_a.y);
                } else if (game->swap_b.x == x && game->swap_b.y == y) {
                    x_offset = swap_distance * (game->swap_a.x - game->swap_b.x);
                    y_offset = swap_distance * (game->swap_a.y - game->swap_b.y);
                }
            }

            switch (tile) {
            case TILE_SHIP:
                game_draw_tile_textured(renderer, game->ship_texture, x, y, x_offset, y_offset);
                break;

            case TILE_LASER:
                game_draw_tile_textured(renderer, game->laser_texture, x, y, x_offset, y_offset);
                break;

            case TILE_ENEMY_LASER:
                game_draw_tile_textured(renderer, game->enemy_laser_texture, x, y, x_offset, y_offset);
                break;

            case TILE_ENEMY:
                game_draw_tile_textured(renderer, game->enemy_texture, x, y, x_offset, y_offset);
                break;

            case TILE_BOMB:
                game_draw_tile_textured(renderer, game->bomb_texture, x, y, x_offset, y_offset);
                break;

            case TILE_ASTEROID_1:
                game_draw_tile_textured(renderer, game->asteroid_1_texture, x, y, x_offset, y_offset);
                break;

            case TILE_ASTEROID_2:
                game_draw_tile_textured(renderer, game->asteroid_2_texture, x, y, x_offset, y_offset);
                break;

            case TILE_ASTEROID_3:
                game_draw_tile_textured(renderer, game->asteroid_3_texture, x, y, x_offset, y_offset);
                break;

            case TILE_EMPTY:
                // An empty tile means that the tiles above will be dropping, calculate the offset.
                // This should only happen in dropping state - we don't want to be swapping and dropping
                // at the same time else the offsets will be messed up.
                assert(game->game_state == GAME_STATE_DROPPING);
                y_offset = (int)(((game->game_time - game->update_time) / DROP_TIME) * TILE_HEIGHT);
                break;

            default:
                assert(!"Unknown tile type");
            }
        }
    }

}

static void
game_event(gamestate_mgr_handle mgr,
           SDL_Event *e,
           game_info_type *game)
{
    coord_type up_coords;

    switch (e->type) {
    case SDL_MOUSEBUTTONDOWN:
        game->mouse_down_coords = game_window_coords_to_tile(e->button.x, e->button.y);
        break;

    case SDL_MOUSEBUTTONUP:
        if (game->game_state == GAME_STATE_IDLE) {
            up_coords = game_window_coords_to_tile(e->button.x, e->button.y);
            if ((up_coords.y == game->mouse_down_coords.y + 1 && up_coords.x == game->mouse_down_coords.x) ||
                (up_coords.y == game->mouse_down_coords.y - 1 && up_coords.x == game->mouse_down_coords.x) ||
                (up_coords.x == game->mouse_down_coords.x + 1 && up_coords.y == game->mouse_down_coords.y) ||
                (up_coords.x == game->mouse_down_coords.x - 1 && up_coords.y == game->mouse_down_coords.y)) {
                game_swap_tiles(game, up_coords, game->mouse_down_coords);
            }
        }
        break;
    }
}


static void
game_cleanup(game_info_type *game)
{
    Mix_FreeChunk(game->match_sound);
    Mix_FreeChunk(game->enemy_shoot_sound);
    Mix_FreeChunk(game->shoot_sound);
    Mix_FreeChunk(game->swap_sound);

    free_texture(game->energy_bar_red_left);
    free_texture(game->energy_bar_red_mid);
    free_texture(game->energy_bar_red_right);
    free_texture(game->energy_bar_yellow_left);
    free_texture(game->energy_bar_yellow_mid);
    free_texture(game->energy_bar_yellow_right);
    free_texture(game->energy_bar_back_left);
    free_texture(game->energy_bar_back_mid);
    free_texture(game->energy_bar_back_right);
    free_texture(game->energy_bar_left);
    free_texture(game->energy_bar_mid);
    free_texture(game->energy_bar_right);
    free_texture(game->enemy_laser_texture);
    free_texture(game->laser_texture);
    free_texture(game->bomb_texture);
    free_texture(game->asteroid_3_texture);
    free_texture(game->asteroid_2_texture);
    free_texture(game->asteroid_1_texture);
    free_texture(game->enemy_texture);
    free_texture(game->ship_texture);

    mapped_font_destroy(game->hud_font_large);
    mapped_font_destroy(game->hud_font);

    free(game);
}


gamestate_type
game_init(SDL_Renderer *renderer)
{
    gamestate_type gamestate;
    game_info_type *game;
    bool updated;
    size_t x;
    size_t y;

    game = calloc(1, sizeof(*game));
    game->renderer = renderer;

    // Load media
    game->hud_font = mapped_font_create(renderer, "media/fonts/hud.ttf", HUD_TEXT_HEIGHT);
    game->hud_font_large = mapped_font_create(renderer, "media/fonts/hud.ttf", HUD_TEXT_LARGE_HEIGHT);

    game->ship_texture = load_texture("media/textures/player.png", renderer);
    game->enemy_texture = load_texture("media/textures/enemyShip.png", renderer);
    game->asteroid_1_texture = load_texture("media/textures/asteroid_1.png", renderer);
    game->asteroid_2_texture = load_texture("media/textures/asteroid_2.png", renderer);
    game->asteroid_3_texture = load_texture("media/textures/asteroid_3.png", renderer);
    game->bomb_texture = load_texture("media/textures/enemyUFO.png", renderer);
    game->laser_texture = load_texture("media/textures/laser.png", renderer);   
    game->enemy_laser_texture = load_texture("media/textures/enemyLaser.png", renderer);
    game->energy_bar_left = load_texture("media/textures/energy_left.png", renderer);
    game->energy_bar_mid = load_texture("media/textures/energy_mid.png", renderer);
    game->energy_bar_right = load_texture("media/textures/energy_right.png", renderer);
    game->energy_bar_back_left = load_texture("media/textures/energy_back_left.png", renderer);
    game->energy_bar_back_mid = load_texture("media/textures/energy_back_mid.png", renderer);
    game->energy_bar_back_right = load_texture("media/textures/energy_back_right.png", renderer);
    game->energy_bar_yellow_left = load_texture("media/textures/energy_yellow_left.png", renderer);
    game->energy_bar_yellow_mid = load_texture("media/textures/energy_yellow_mid.png", renderer);
    game->energy_bar_yellow_right = load_texture("media/textures/energy_yellow_right.png", renderer);
    game->energy_bar_red_left = load_texture("media/textures/energy_red_left.png", renderer);
    game->energy_bar_red_mid = load_texture("media/textures/energy_red_mid.png", renderer);
    game->energy_bar_red_right = load_texture("media/textures/energy_red_right.png", renderer);

    game->swap_sound = Mix_LoadWAV("media/sounds/swap.ogg");
    game->shoot_sound = Mix_LoadWAV("media/sounds/shoot.ogg");
    game->enemy_shoot_sound = Mix_LoadWAV("media/sounds/enemy_shoot.ogg");
    game->match_sound = Mix_LoadWAV("media/sounds/match.ogg");

    // Set up board.
    for (x = 0; x < BOARD_WIDTH; x++) {
        game->next_row[x] = game_random_tile();

        for (y = 0; y < BOARD_HEIGHT; y++) {
            game->tiles[x][y] = game_random_tile();
        }
    }

    do {
        updated = false;

        // Check for matches
        game_check_board(game, false);

        // Fill in any gaps.
        for (x = 0; x < BOARD_WIDTH; x++) {
            for (y = 0; y < BOARD_HEIGHT; y++) {
                if (game->tiles[x][y] == TILE_EMPTY) {
                    updated = true;
                    game->tiles[x][y] = game_random_tile();
                }
            }
        }
    } while (updated);
    
    game->chain = 1;
    game->score = 0;
    game->game_state = GAME_STATE_IDLE;
    game->energy = MAX_ENERGY;
    game->tick_time = ENERGY_TICK_TIME;

    gamestate.update_cb = (gamestate_update_fn_type)&game_update;
    gamestate.draw_cb = (gamestate_draw_fn_type)&game_draw;
    gamestate.event_cb = (gamestate_event_fn_type)&game_event;
    gamestate.cleanup_cb = (gamestate_cleanup_fn_type)&game_cleanup;
    gamestate.flags = GAMESTATE_FLAG_DEFAULT;
    gamestate.ctx = game;

    return gamestate;
}