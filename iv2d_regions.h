#pragma once

#include "iv2d.h"

struct iv2d_circle {
    struct iv2d_region region;
    float              x,y,r, padding;
};
iv iv2d_circle(struct iv2d_region const*, iv x, iv y);

struct iv2d_capsule {
    struct iv2d_region region;
    float              x0,y0,x1,y1,r, padding;
};
iv iv2d_capsule(struct iv2d_region const*, iv x, iv y);

struct iv2d_setop {
    struct iv2d_region         region;
    struct iv2d_region const **subregion;
    int                        subregions, padding;
};
iv iv2d_union    (struct iv2d_region const*, iv x, iv y);
iv iv2d_intersect(struct iv2d_region const*, iv x, iv y);

struct iv2d_invert {
    struct iv2d_region        region;
    struct iv2d_region const *arg;
};
iv iv2d_invert(struct iv2d_region const*, iv x, iv y);

struct iv2d_stroke {
    struct iv2d_region        region;
    struct iv2d_region const *arg;
    float                     width,padding;
};
iv iv2d_stroke(struct iv2d_region const*, iv x, iv y);

struct iv2d_halfplane {
    struct iv2d_region region;
    float              nx,ny,d,padding;
};
iv iv2d_halfplane(struct iv2d_region const*, iv x, iv y);
