#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

extern struct slide capsule_slide;

struct capsule_data {
    struct iv2d_capsule capsule;
};

static struct iv2d_region const* create(float const *w, float const *h, float const *t) {
    struct capsule_data *d = malloc(sizeof *d);
    float W=*w, H=*h, T=*t;
    float cx = 0.5f * W,
          cy = 0.5f * H;
    float ox = cx + (300 - cx)*cosf(T) - (200 - cy)*sinf(T);
    float oy = cy + (200 - cy)*cosf(T) + (300 - cx)*sinf(T);
    d->capsule = (struct iv2d_capsule){.region={iv2d_capsule}, ox,oy, cx,cy, 4};
    (void)H; // unused except in expressions above
    return &d->capsule.region;
}

static void cleanup(struct iv2d_region const *region) {
    free((void*)(uintptr_t)region);
}

struct slide capsule_slide = {"capsule", create, cleanup};
