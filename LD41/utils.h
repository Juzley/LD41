#ifndef __UTILS_H__
#define __UTILS_H__


#include <math.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>


#define DEG_TO_RAD(angle) ((angle) * M_PI / 180.0)
#define RAD_TO_DEG(angle) ((angle) * 180.0 / M_PI)

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


static SDL_Color color_white = { 255, 255, 255, 255 };
static SDL_Color color_red = { 255, 0, 0, 255 };

void rect_center_int(SDL_Rect *rect, int *x, int *y);
void rect_center_float(SDL_Rect *rect, float *x, float *y);
void rotate_point(int x, int y, float angle, int origin_x, int origin_y, int *rotated_x, int *rotated_y);
void draw_rotated_rect(SDL_Renderer *renderer, SDL_Rect *rect, float angle);

static inline SDL_Texture *
load_texture(const char   *filename,
             SDL_Renderer *renderer)
{
    SDL_Surface *surf;
    SDL_Texture *result = NULL;

    surf = IMG_Load(filename);
    if (surf != NULL) {
        result = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
    }

    return result;
}

static inline void
free_texture(SDL_Texture *texture)
{
    SDL_DestroyTexture(texture);
}

static inline void
draw_overlay(SDL_Renderer *renderer, int screen_width, int screen_height)
{
    SDL_Rect rect = { .x = 0,.y = 0,.w = screen_width,.h = screen_height };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &rect);
}

/*
* Find a random number in the closed interval [min, max].
* Assumes 0 <= max <= RAND_MAX
*/
unsigned int random_range(unsigned int min, unsigned int max);


#endif /* __UTILS_H__ */