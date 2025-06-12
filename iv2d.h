#pragma once

#include "iv.h"
#include <stddef.h>

// A 2D region represented as a signed distance function of x and y.
// The boundary of the region is eval(x,y)==0, with eval(x,y)<=0 for area within the region.
struct iv2d_region {
    iv (*eval)(struct iv2d_region const*, void* scratch, iv x, iv y);
    size_t scratch;
};

// iv2d_cover rasterizes an iv2d_region onto a bounded integer grid, yielding
// [0,1] coverage values for rectangles within {l,t,r,b} that overlap the region.
//
// The quality parameter controls how much attention iv2d_cover pays to pixels
// overlapping the region's boundary.  If quality <= 0, it ignores them, and so
// never yields rectangles with anything less than full 1.0f coverage; results
// will be pixelated.  At quality 1, it spends minor effort estimating those
// partial coverages, and each higher level of quality roughly doubles that
// effort.
void iv2d_cover(struct iv2d_region const*, int l, int t, int r, int b, int quality,
                void (*yield)(void*, float l, float t, float r, float b, float cov), void *ctx);
