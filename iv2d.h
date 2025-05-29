#pragma once

#include "iv.h"

// A 2D region represented as an expression of X and Y.
// The boundary of the region is f(X,Y)==0,
// with f(X,Y)<0 for area within the region.
typedef iv4 iv2d_region(void const *ctx, iv4 X, iv4 Y);

// iv2d_circle is a sample region, a circle centered at x,y with radius r.
//
//   The typical equation for the boundary of a circle looks like
//         (X - c.x)^2 + (Y - c.y)^2 == c.r^2
//
//   and the area inside that circle is described by the inequality
//         (X - c.x)^2 + (Y - c.y)^2 <  c.r^2
//
//   so iv2d_circle() returns the value
//         (X - c.x)^2 + (Y - c.y)^2 -  c.r^2
struct iv2d_circle { float x,y,r; };
iv2d_region iv2d_circle;

struct iv2d_binop {
    iv2d_region *a; void const *actx;
    iv2d_region *b; void const *bctx;
};
iv2d_region iv2d_union, iv2d_intersection, iv2d_difference;

// iv2d_cover rasterizes an iv2d_region onto a bounded integer grid, yielding
// [0,1] coverage values for rectangles within bounds that overlap the region.
//
// The quality parameter controls how much attention iv2d_cover pays to pixels
// overlapping the region's boundary.  If quality <= 0, it ignores them, and so
// never yields rectangles with anything less than full 1.0f coverage; results
// will be pixelated.  At quality 1, it spends minor effort estimating those
// partial coverages, and each higher level of quality roughly doubles that
// effort.
void iv2d_cover(iv2d_region*, void const *ctx,
                int l, int t, int r, int b, int quality,
                void (*yield)(void *arg, float l, float t, float r, float b, float cov), void *arg);

