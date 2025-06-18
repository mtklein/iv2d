#pragma once
#include "iv2d.h"
#include <stdlib.h>
#pragma clang diagnostic ignored "-Wpadded"

struct slide {
    const char *name;
    struct iv2d_region const *region;
    struct iv2d_region const *(*make)(struct slide *, float w, float h, double t);
    int lazy; // if non-zero, cache the region
};

static inline struct iv2d_region const *slide_region(struct slide *s, float w, float h, double t) {
    if (!s->lazy || s->region == NULL) {
        s->region = s->make(s, w, h, t);
    }
    return s->region;
}

extern struct slide union_slide;
extern struct slide intersect_slide;
extern struct slide difference_slide;
extern struct slide capsule_slide;
extern struct slide halfplane_slide;
extern struct slide ngon_slide;
extern struct slide vm_union_slide;
extern struct slide prospero_slide;

extern struct slide *slides[];
extern int slide_count;
