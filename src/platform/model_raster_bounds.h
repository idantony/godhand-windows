#ifndef GH_MODEL_RASTER_BOUNDS_H
#define GH_MODEL_RASTER_BOUNDS_H

#include <stdint.h>

typedef struct { uint32_t x, y, width, height; } gh_model_raster_bounds;

/* The shader samples at (x*16,y*16), on the 16.4 vertex grid. Clip the
 * closed vertex bounds to those sample coordinates and the viewport. */
static inline gh_model_raster_bounds gh_model_raster_bounds_from_fixed4(
    int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y,
    uint32_t surface_width, uint32_t surface_height)
{
    uint32_t x0=min_x>0?((uint32_t)min_x+15u)/16u:0;
    uint32_t y0=min_y>0?((uint32_t)min_y+15u)/16u:0;
    uint32_t x1=max_x>=0?(uint32_t)max_x/16u+1u:0;
    uint32_t y1=max_y>=0?(uint32_t)max_y/16u+1u:0;
    if(x0>surface_width)x0=surface_width;
    if(y0>surface_height)y0=surface_height;
    if(x1>surface_width)x1=surface_width;
    if(y1>surface_height)y1=surface_height;
    gh_model_raster_bounds result={x0,y0,x1>x0?x1-x0:0,y1>y0?y1-y0:0};
    return result;
}

#endif
