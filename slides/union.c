#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

static struct iv2d_region const *make_union(struct slide *s, float w, float h, double t) {
    (void)s;
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float cr = 0.5f * fminf(cx, cy);
    float th = (float)t;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    static struct iv2d_circle center, orbit;
    static struct iv2d_region const *sub[2];
    static struct iv2d_setop op;

    center = (struct iv2d_circle){.region={iv2d_circle}, cx, cy, cr};
    orbit  = (struct iv2d_circle){.region={iv2d_circle}, ox, oy, 100};
    sub[0] = &center.region;
    sub[1] = &orbit.region;
    op = (struct iv2d_setop){.region={iv2d_union}, sub, 2};
    return &op.region;
}

struct slide union_slide = {"union", NULL, make_union, 0};
