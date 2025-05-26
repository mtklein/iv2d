#pragma once

#include "iv.h"

struct iv2d_rect {
    int l,t,r,b;
};

struct iv2d_edge {
    iv (*fn)(void const *iv2d_edge, iv x, iv y);
};

struct iv2d_cover_yield {
    void (*fn)(struct iv2d_rect bounds, float cov, void *ctx);
    void  *ctx;
};

void iv2d_cover(struct iv2d_rect bounds, struct iv2d_edge const*, struct iv2d_cover_yield);

struct iv2d_circle {
    struct iv2d_edge edge;
    float            x,y,r,padding;
};
struct iv2d_circle iv2d_circle(float x, float y, float r);
