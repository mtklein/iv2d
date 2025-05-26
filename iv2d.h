#pragma once

#include "iv.h"

struct iv2d_rect {
    int l,t,r,b;
};

struct iv2d_edge {
    iv        (*fn)(iv x, iv y, void const *ctx);
    void const *ctx;
};

struct iv2d_cover_yield {
    void (*fn)(struct iv2d_rect bounds, float cov, void *ctx);
    void  *ctx;
};

void iv2d_cover(struct iv2d_rect bounds, struct iv2d_edge, struct iv2d_cover_yield);

struct iv2d_circle {
    float x,y,r;
};
iv iv2d_circle(iv x, iv y, void const *iv2d_circle);
