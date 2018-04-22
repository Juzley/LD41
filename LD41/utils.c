#include <stdbool.h>
#include <stdlib.h>
#include <SDL.h>


void
rect_center_int(SDL_Rect *rect, int *x, int *y)
{
    *x = (int)(rect->x + rect->w / 2.0f);
    *y = (int)(rect->y + rect->h / 2.0f);
}


void
rect_center_float(SDL_Rect *rect, float *x, float *y)
{
    *x = rect->x + rect->w / 2.0f;
    *y = rect->y + rect->h / 2.0f;
}


void
rotate_point(int x, int y, float angle, int origin_x, int origin_y, int *rotated_x, int *rotated_y)
{
    x -= origin_x;
    y -= origin_y;
    *rotated_x = (int)(x * cosf(angle) - y * sinf(angle)) + origin_x;
    *rotated_y = (int)(x * sinf(angle) + y * cosf(angle)) + origin_y;
}


void
draw_rotated_rect(SDL_Renderer *renderer,
                  SDL_Rect     *rect,
                  float         angle)
{
    int       origin_x;
    int       origin_y;
    SDL_Point points[5];

    rect_center_int(rect, &origin_x, &origin_y);

    rotate_point(rect->x, rect->y, angle, origin_x, origin_y, &points[0].x, &points[0].y);
    rotate_point(rect->x + rect->w - 1, rect->y, angle, origin_x, origin_y, &points[1].x, &points[1].y);
    rotate_point(rect->x + rect->w - 1, rect->y + rect->h - 1, angle, origin_x, origin_y, &points[2].x, &points[2].y);
    rotate_point(rect->x, rect->y + rect->h - 1, angle, origin_x, origin_y, &points[3].x, &points[3].y);
    rotate_point(rect->x, rect->y, angle, origin_x, origin_y, &points[4].x, &points[4].y);

    (void)SDL_RenderDrawLines(renderer, points, 5);
}


/*
 * See utils.h for details.
 */
unsigned int
random_range(unsigned int min, unsigned int max) {
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;


    /* Create equal size buckets all in a row, then fire randomly towards
    * the buckets until you land in one of them. All buckets are equally
    * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}