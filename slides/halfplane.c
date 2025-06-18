#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

extern struct slide halfplane_slide;

static struct iv2d_halfplane halfplane_from(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0,
          dy = y1 - y0,
          norm = 1.0f / sqrtf(dx*dx + dy*dy),
          nx = +dy*norm,
          ny = -dx*norm,
          d  = nx*x0 + ny*y0;
    return (struct iv2d_halfplane){.region={iv2d_halfplane}, nx,ny,d};
}

struct hp_data {
    struct iv2d_halfplane hp;
};

static struct iv2d_region const* create(float w, float h, float t) {
    struct hp_data *d = malloc(sizeof *d);
    float cx = 0.5f * w,
          cy = 0.5f * h;
    float ox = cx + (300 - cx)*cosf(t) - (200 - cy)*sinf(t);
    float oy = cy + (200 - cy)*cosf(t) + (300 - cx)*sinf(t);
    d->hp = halfplane_from(ox,oy,cx,cy);
    return &d->hp.region;
}

static void cleanup(struct iv2d_region const *region) {
    free((void*)(uintptr_t)region);
}

struct slide halfplane_slide = {"halfplane", create, cleanup};
