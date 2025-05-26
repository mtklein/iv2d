#pragma once

#include "iv.h"

struct iv2d_rect {
    int l,t,r,b;
};

void iv2d_cover(struct iv2d_rect bounds,
                iv (*edge)(iv x, iv y, void const *ectx), void const *ectx,
                void (*yield)(struct iv2d_rect bounds, float c, void *yctx), void *yctx);

struct iv2d_circle {
    float x,y,r;
};
iv iv2d_circle(iv x, iv y, void const *iv2d_circle);
