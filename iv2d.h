#pragma once

#include "iv.h"

// A 2D region represented as an expression of X and Y.
// The boundary of the region is fn(X,Y)==0,
// with fn(X,Y)<0 for area within the region.
struct iv2d_region {
    iv (*fn)(struct iv2d_region const*, iv X, iv Y);
};

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
struct iv2d_circle {
    struct iv2d_region region;

    float x,y,r,padding;
};
struct iv2d_circle iv2d_circle(float x, float y, float r);

struct iv2d_rect {
    int l,t,r,b;
};

// iv2d_cover rasterizes an iv2d_region onto a bounded integer grid, yielding
// [0,1] coverage values for rectangles within bounds that overlap the region.
//
// iv2d_cover yields rectangles' coverages to your callback in an unspecified
// order, subject to change.  The rectangles may be any size, from single
// pixels all the way up to the entire bounds.
//
// The quality parameter controls how much attention iv2d_cover pays to
// rectangles overlapping the region's boundary.  If quality <= 0, it ignores
// them, and so never yields rectangles with anything less than full 1.0f
// coverage; results will be pixelated.  At quality 1, it spends minimal effort
// estimating those partial coverages, and higher levels of quality increase
// that effort.  While antialiasing is a matter of taste, I'd describe
// quality=1 as good, quality=2 as great, and anything higher as overkill.
struct iv2d_coverage_cb {
    void (*fn)(struct iv2d_coverage_cb*, struct iv2d_rect bounds, float coverage);
};
void iv2d_cover(struct iv2d_rect bounds,
                int              quality,
                struct iv2d_region const*,
                struct iv2d_coverage_cb*);

