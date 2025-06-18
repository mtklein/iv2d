#include "../iv2d_regions.h"
#include "slides.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

extern struct slide ngon_slide;

#define HP 7

static struct iv2d_halfplane halfplane_from(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0,
          dy = y1 - y0,
          norm = 1.0f / sqrtf(dx*dx + dy*dy),
          nx = +dy*norm,
          ny = -dx*norm,
          d  = nx*x0 + ny*y0;
    return (struct iv2d_halfplane){.region={iv2d_halfplane}, nx,ny,d};
}

struct ngon_data {
    struct iv2d_setop op;
    struct iv2d_region const* sub[HP];
    struct iv2d_halfplane hp[HP];
};

static struct iv2d_region const* create(float const *w, float const *h, float const *t) {
    struct ngon_data *d = malloc(sizeof *d);
    float W=*w, H=*h, T=*t;
    float cx = 0.5f * W,
          cy = 0.5f * H;
    float pi = (float)atan(1) * 4;
    float step = 2 * pi / (float)HP;
    for (int i = 0; i < HP; i++) {
        float a0 = T + (float)i * step;
        float a1 = T + (float)(i+1) * step;
        d->hp[i] = halfplane_from(cx + 100 * cosf(a0),
                                  cy + 100 * sinf(a0),
                                  cx + 100 * cosf(a1),
                                  cy + 100 * sinf(a1));
        d->sub[i] = &d->hp[i].region;
    }
    d->op = (struct iv2d_setop){.region={iv2d_intersect}, d->sub, HP};
    return &d->op.region;
}

static void cleanup(struct iv2d_region const *region) {
    free((void*)(uintptr_t)region);
}

struct slide ngon_slide = {"ngon", create, cleanup};
