#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL2/SDL_ttf.h>
#include "font.h"
#include "utils.h"

#define MIN_CHAR 0x20
#define MAX_CHAR 0x7E
#define CHAR_COUNT (MAX_CHAR - MIN_CHAR + 1)
#define CHAR_INDEX(c) ((c) - MIN_CHAR)

typedef struct mapped_font {
    SDL_Rect     map[CHAR_COUNT];
    SDL_Texture *texture;
} mapped_font_type;


static inline bool
mapped_font_char_ok(char c)
{
    return c >= MIN_CHAR && c <= MAX_CHAR;
}


static inline void
mapped_font_ttf_char_bounds(TTF_Font *font,
                            char      c,
                            int      *width,
                            int      *height)
{
    char buf[] = { c, 0 };
    TTF_SizeText(font, buf, width, height);
}


static inline SDL_Surface *
mapped_font_ttf_char_render(TTF_Font *font,
                            char       c)
{
    SDL_Color white = { 255, 255, 255, 255 };
    char      buf[] = { c, 0 };

    return TTF_RenderText_Blended(font, buf, white);
}


static SDL_Surface *
mapped_font_create_surface(int width,
                           int height)
{
    SDL_Surface *surface;
    Uint32       rmask;
    Uint32       gmask;
    Uint32       bmask;
    Uint32       amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    surface = SDL_CreateRGBSurface(0, width, height, 32,  rmask, gmask, bmask, amask);
    if (surface == NULL) {
        SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
    }

    return surface;
}



mapped_font_handle
mapped_font_create(SDL_Renderer *renderer,
                   const char   *filename,
                   int           height)
{
    mapped_font_handle  result = NULL;
    TTF_Font           *font = NULL;
    SDL_Surface        *surf;
    SDL_Surface        *overall_surf;
    int                 texture_width = 0;
    int                 texture_height = 0;
    int                 char_width;
    int                 char_height;
    char                c;
    bool                ok = true;

    if (ok) {
        result = calloc(1, sizeof(*result));
        if (result == NULL) {
            ok = false;
        }
    }

    if (ok) {
        font = TTF_OpenFont(filename, height);
        if (font == NULL) {
            ok = false;
        }
    }

    if (ok) {
        // Work out the size of the final texture and create a surface that can hold all the characters.
        for (c = MIN_CHAR; c <= MAX_CHAR; c++) {
            mapped_font_ttf_char_bounds(font, c, &char_width, &char_height);
            result->map[CHAR_INDEX(c)].x = texture_width;
            result->map[CHAR_INDEX(c)].y = 0;
            result->map[CHAR_INDEX(c)].w = char_width;
            result->map[CHAR_INDEX(c)].h = char_height;

            texture_width += char_width;
            texture_height = char_height > texture_height ? char_height : texture_height;
        }

        overall_surf = mapped_font_create_surface(texture_width, texture_height);
        if (overall_surf == NULL) {
            ok = false;
        }
    }

    
    if (ok) {
        // Render characters into the overall surface.
        for (c = MIN_CHAR; c <= MAX_CHAR; c++) {
            surf = mapped_font_ttf_char_render(font, c);
            (void)SDL_BlitSurface(surf, NULL, overall_surf, &result->map[CHAR_INDEX(c)]);
            SDL_FreeSurface(surf);
        }
    }

    if (ok) {
        // Create a texture from the overall surface.
        result->texture = SDL_CreateTextureFromSurface(renderer, overall_surf);
        if (result->texture == NULL) {
            ok = false;
        }
    }

    if (ok) {
        // Set the blendmode for the new texture.
        SDL_SetTextureBlendMode(result->texture, SDL_BLENDMODE_BLEND);
    }

    TTF_CloseFont(font);
    SDL_FreeSurface(overall_surf);
    if (!ok && result != NULL) {
        SDL_DestroyTexture(result->texture);
        free(result);
        result = NULL;
    }

    return result;
}


void
mapped_font_destroy(mapped_font_handle font)
{
    SDL_DestroyTexture(font->texture);
    free(font);
}


void
mapped_font_draw(SDL_Renderer       *renderer,
                 mapped_font_handle  font,
                 int                 x,
                 int                 y,
                 const char         *text)
{
    SDL_Color white = { 255, 255, 255, 255 };
    mapped_font_draw_ex(renderer, font, x, y, 0, 0, 0, color_white, ALIGN_LEFT, text);
}


void
mapped_font_draw_ex(SDL_Renderer           *renderer,
                    mapped_font_handle      font,
                    int                     x,
                    int                     y,
                    float                   angle,
                    int                     origin_x,
                    int                     origin_y,
                    SDL_Color               color,
                    mapped_font_align_type  align,
                    const char             *text)
{
    SDL_Point origin = { 0, 0 };
    SDL_Rect  rect;
    char      c;
    int       width;
    int       height;

    SDL_SetTextureColorMod(font->texture, color.r, color.g, color.b);

    switch (align) {
    case ALIGN_LEFT:
        break;

    case ALIGN_CENTER:
        mapped_font_bounds(font, text, &width, &height);
        x -= width / 2;
        break;

    case ALIGN_RIGHT:
        mapped_font_bounds(font, text, &width, &height);
        x -= width;
        break;
    }

    for (; *text != '\0'; text++) {
        c = *text;

        // If the char is outside the printable range, just skip it.
        if (!mapped_font_char_ok(c)) {
            continue;
        }
           
        rect.x = x;
        rect.y = y;
        origin.x = origin_x - rect.x;
        origin.y = origin_y - rect.y;
        rect.w = font->map[CHAR_INDEX(c)].w;
        rect.h = font->map[CHAR_INDEX(c)].h;

        SDL_RenderCopyEx(renderer, font->texture, &font->map[CHAR_INDEX(c)], &rect, RAD_TO_DEG(angle), &origin, SDL_FLIP_NONE);

        x += rect.w;
    }
}


void
mapped_font_bounds(mapped_font_handle  font,
                   const char         *text,
                   int                *width,
                   int                *height)
{
    SDL_Rect *char_bounds;

    for (*width = 0, *height = 0; *text != '\0'; text++) {
        if (!mapped_font_char_ok(*text)) {
            continue;
        }

        char_bounds = &font->map[CHAR_INDEX(*text)];
        *width += char_bounds->w;
        *height = char_bounds->h > *height ? char_bounds->h : *height;
    }
}


void
mapped_font_drawf(SDL_Renderer       *renderer,
                  mapped_font_handle  font,
                  int                 x,
                  int                 y,
                  const char         *fmt,
                  ...)
{
    char buf[256];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 256, fmt, args);
    mapped_font_draw(renderer, font, x, y, buf);
    va_end(args);
}


void
mapped_font_drawf_ex(SDL_Renderer           *renderer,
                     mapped_font_handle      font,
                     int                     x,
                     int                     y,
                     mapped_font_align_type  align,
                     const char             *fmt,
                     ...)
{
    char buf[256];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 256, fmt, args);
    mapped_font_draw_ex(renderer, font, x, y, 0, 0, 0, color_white, align, buf);
    va_end(args);
}

