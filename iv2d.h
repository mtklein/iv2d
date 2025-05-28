#pragma once

#include "iv.h"

// A 2D region represented as an expression of X and Y.
// The boundary of the region is f(X,Y)==0,
// with f(X,Y)<0 for area within the region.
typedef iv4 iv2d_region(iv4 X, iv4 Y, void const *ctx);

// iv2d_circle is a sample region, a circle centered at x,y with radius r.
//
// The typical equation for the boundary of a circle looks like
//         (X - c.x)^2 + (Y - c.y)^2 == c.r^2
//
// so the area inside that circle is described by the inequality
//         (X - c.x)^2 + (Y - c.y)^2 <  c.r^2
//
// which all means this region will return the value
//         (X - c.x)^2 + (Y - c.y)^2 -  c.r^2
struct iv2d_circle { float x,y,r; };
iv2d_region iv2d_circle;

struct iv2d_binop {
    iv2d_region *a; void const *actx;
    iv2d_region *b; void const *bctx;
};
iv2d_region iv2d_union, iv2d_intersection, iv2d_difference;

struct iv2d_rect {
    int l,t,r,b;
};

// iv2d_cover rasterizes an iv2d_region onto a bounded integer grid, yielding
// [0,1] coverage values for rectangles within bounds that overlap the region.
//
// The quality parameter controls how much attention iv2d_cover pays to
// rectangles overlapping the region's boundary.  If quality <= 0, it ignores
// them, and so never yields rectangles with anything less than full 1.0f
// coverage; results will be pixelated.  At quality 1, it spends minimal effort
// estimating those partial coverages, and each higher level of quality roughly
// doubles that effort.  I'd describe quality=1 as good, quality=2 as great,
// and quality=4 near perfect.
//
// iv2d_cover yields rectangles' coverages to your callback in an unspecified
// order, subject to change.  The rectangles may be any size, from single
// pixels all the way up to the entire bounds.
void iv2d_cover(iv2d_region*, void const *ctx,
                struct iv2d_rect bounds, int quality,
                void (*yield)(struct iv2d_rect, float, void *yield_ctx), void *yield_ctx);

