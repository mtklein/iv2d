#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>

extern struct slide difference_slide;

struct diff_data {
    struct iv2d_setop op;
    struct iv2d_region const* sub[2];
    struct iv2d_circle center, orbit;
    struct iv2d_invert invorb;
};

static struct iv2d_region const* create(float w, float h, float t) {
    struct diff_data *d = malloc(sizeof *d);
    float cx = 0.5f * w,
          cy = 0.5f * h,
          cr = 0.5f * fminf(cx,cy);
    float ox = cx + (300 - cx)*cosf(t) - (200 - cy)*sinf(t);
    float oy = cy + (200 - cy)*cosf(t) + (300 - cx)*sinf(t);
    d->center = (struct iv2d_circle){.region={iv2d_circle}, cx,cy,cr};
    d->orbit  = (struct iv2d_circle){.region={iv2d_circle}, ox,oy,100};
    d->invorb = (struct iv2d_invert){.region={iv2d_invert}, &d->orbit.region};
    d->sub[0] = &d->center.region;
    d->sub[1] = &d->invorb.region;
    d->op = (struct iv2d_setop){.region={iv2d_intersect}, d->sub, 2};
    return &d->op.region;
}

struct slide difference_slide = {"difference", create};
