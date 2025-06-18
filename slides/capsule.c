#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

extern struct slide capsule_slide;

struct capsule_data {
    struct iv2d_capsule capsule;
};

static struct iv2d_region const* create(float w, float h, float t) {
    struct capsule_data *d = malloc(sizeof *d);
    float cx = 0.5f * w,
          cy = 0.5f * h;
    float ox = cx + (300 - cx)*cosf(t) - (200 - cy)*sinf(t);
    float oy = cy + (200 - cy)*cosf(t) + (300 - cx)*sinf(t);
    d->capsule = (struct iv2d_capsule){.region={iv2d_capsule}, ox,oy, cx,cy, 4};
    (void)h; // unused except in expressions above
    return &d->capsule.region;
}

static void cleanup(struct iv2d_region const *region) {
    free((void*)(uintptr_t)region);
}

struct slide capsule_slide = {"capsule", create, cleanup};
