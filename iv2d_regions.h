#pragma once

#include "iv2d.h"

struct iv2d_circle {
    struct iv2d_region region;
    float              x,y,r, padding;
};
iv32 iv2d_circle(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_capsule {
    struct iv2d_region region;
    float              x0,y0,x1,y1,r, padding;
};
iv32 iv2d_capsule(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_setop {
    struct iv2d_region         region;
    struct iv2d_region const **subregion;
    int                        subregions, padding;
};
iv32 iv2d_union    (struct iv2d_region const*, iv32 x, iv32 y);
iv32 iv2d_intersect(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_invert {
    struct iv2d_region        region;
    struct iv2d_region const *arg;
};
iv32 iv2d_invert(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_stroke {
    struct iv2d_region        region;
    struct iv2d_region const *arg;
    float                     width, padding;
};
iv32 iv2d_stroke(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_halfplane {
    struct iv2d_region region;
    float              nx,ny,d, padding;
};
iv32 iv2d_halfplane(struct iv2d_region const*, iv32 x, iv32 y);
