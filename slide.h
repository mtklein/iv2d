#pragma once
#include "iv2d.h"
#include "len.h"
#include <stdlib.h>

struct slide {
    const char *name;
    struct iv2d_region const *region;
    struct iv2d_region const *(*make)(struct slide *, float const *w, float const *h, float const *t);
    void (*cleanup)(struct slide *);
    void *data;
    void *padding;
};

static inline struct iv2d_region const *slide_region(struct slide *s, float const *w, float const *h, float const *t) {
    if (!s->region) {
        s->region = s->make(s, w, h, t);
    }
    return s->region;
}

static inline void slide_cleanup(struct slide *s) {
    if (s->cleanup) {
        s->cleanup(s);
    }
    s->region = NULL;
}

extern struct slide union_slide;
extern struct slide intersect_slide;
extern struct slide difference_slide;
extern struct slide capsule_slide;
extern struct slide halfplane_slide;
extern struct slide ngon_slide;
extern struct slide vm_union_slide;
extern struct slide prospero_slide;

static struct slide *slide[] = {
    &union_slide,
    &intersect_slide,
    &difference_slide,
    &capsule_slide,
    &halfplane_slide,
    &ngon_slide,
    &vm_union_slide,
    &prospero_slide,
};
enum { slides = len(slide) };
