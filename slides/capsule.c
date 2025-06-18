#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>

extern struct slide capsule_slide;

static struct iv2d_region const* create(float w, float h, float t) {
    struct iv2d_capsule *cap = malloc(sizeof *cap);
    float cx = 0.5f * w,
          cy = 0.5f * h;
    float ox = cx + (300 - cx)*cosf(t) - (200 - cy)*sinf(t);
    float oy = cy + (200 - cy)*cosf(t) + (300 - cx)*sinf(t);
    *cap = (struct iv2d_capsule){.region={iv2d_capsule}, ox,oy, cx,cy, 4};
    return &cap->region;
}

struct slide capsule_slide = {"capsule", create};
