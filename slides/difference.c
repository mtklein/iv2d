#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

extern struct slide difference_slide;

struct diff_data {
    struct iv2d_setop op;
    struct iv2d_region const* sub[2];
    struct iv2d_circle center, orbit;
    struct iv2d_invert invorb;
};

static struct iv2d_region const* create(float const *w, float const *h, float const *t) {
    struct diff_data *d = malloc(sizeof *d);
    float W=*w, H=*h, T=*t;
    float cx = 0.5f * W,
          cy = 0.5f * H,
          cr = 0.5f * fminf(cx,cy);
    float ox = cx + (300 - cx)*cosf(T) - (200 - cy)*sinf(T);
    float oy = cy + (200 - cy)*cosf(T) + (300 - cx)*sinf(T);
    d->center = (struct iv2d_circle){.region={iv2d_circle}, cx,cy,cr};
    d->orbit  = (struct iv2d_circle){.region={iv2d_circle}, ox,oy,100};
    d->invorb = (struct iv2d_invert){.region={iv2d_invert}, &d->orbit.region};
    d->sub[0] = &d->center.region;
    d->sub[1] = &d->invorb.region;
    d->op = (struct iv2d_setop){.region={iv2d_intersect}, d->sub, 2};
    return &d->op.region;
}

static void cleanup(struct iv2d_region const *region) {
    free((void*)(uintptr_t)region);
}

struct slide difference_slide = {"difference", create, cleanup};
