#ifndef __FONT_H__
#define __FONT_H__


#include <SDL.h>
#include <SDL2/SDL_ttf.h>

typedef struct mapped_font *mapped_font_handle;


typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
} mapped_font_align_type;


mapped_font_handle mapped_font_create(SDL_Renderer *renderer,
                                      const char   *filename,
                                      int           height);
void mapped_font_destroy(mapped_font_handle font);

void mapped_font_draw(SDL_Renderer       *renderer,
                      mapped_font_handle  font,
                      int                 x,
                      int                 y,
                      const char         *text);

void mapped_font_draw_ex(SDL_Renderer       *renderer,
                         mapped_font_handle  font,
                         int                 x,
                         int                 y,
                         float               angle,
                         int                 origin_x,
                         int                 origin_y,
                         SDL_Color           color,
                         mapped_font_align_type align,
                         const char         *text);

void mapped_font_drawf(SDL_Renderer       *renderer,
                       mapped_font_handle  font,
                       int                 x,
                       int                 y,
                       const char         *fmt,
                       ...);

void mapped_font_drawf_ex(SDL_Renderer           *renderer,
                          mapped_font_handle      font,
                          int                     x,
                          int                     y,
                          mapped_font_align_type  align,
                          const char             *fmt,
                          ...);

void mapped_font_bounds(mapped_font_handle  font,
                        const char         *text,
                        int                *width,
                        int                *height);


#endif /* __FONT_H__ */
