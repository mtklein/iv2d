#pragma once

#include "iv2d.h"

struct iv2d_circle {
    struct iv2d_region region;
    float              x,y,r;
    int                :32;
};
iv32 iv2d_circle(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_capsule {
    struct iv2d_region region;
    float              x0,y0,x1,y1,r;
    int                :32;
};
iv32 iv2d_capsule(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_setop {
    struct iv2d_region         region;
    struct iv2d_region const **subregion;
    int                        subregions;
    int                        :32;
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
    float                     width;
    int                       :32;
};
iv32 iv2d_stroke(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_halfplane {
    struct iv2d_region region;
    float              nx,ny,d;
    int                :32;
};
iv32 iv2d_halfplane(struct iv2d_region const*, iv32 x, iv32 y);

struct iv2d_sdf {
    struct iv2d_region region;
    _Float16 const    *sdf;
    float              x,y;
    int                w,h;
};
iv32 iv2d_sdf(struct iv2d_region const*, iv32 x, iv32 y);
