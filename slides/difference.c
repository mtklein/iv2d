#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

static struct iv2d_region const *make_difference(struct slide *s, float w, float h, double t) {
    (void)s;
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float cr = 0.5f * fminf(cx, cy);
    float th = (float)t;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    static struct iv2d_circle center, orbit;
    static struct iv2d_invert inv;
    static struct iv2d_region const *sub[2];
    static struct iv2d_setop op;

    center = (struct iv2d_circle){.region={iv2d_circle}, cx, cy, cr};
    orbit  = (struct iv2d_circle){.region={iv2d_circle}, ox, oy, 100};
    inv    = (struct iv2d_invert){.region={iv2d_invert}, &orbit.region};
    sub[0] = &center.region;
    sub[1] = &inv.region;
    op = (struct iv2d_setop){.region={iv2d_intersect}, sub, 2};
    return &op.region;
}

struct slide difference_slide = {"difference", NULL, make_difference, 0};
