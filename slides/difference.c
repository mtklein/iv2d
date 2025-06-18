#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

struct difference_data {
    struct iv2d_circle center, orbit;
    struct iv2d_invert inv;
    struct iv2d_region const *sub[2];
    struct iv2d_setop op;
};

static void cleanup_difference(struct slide *s) {
    free(s->data);
    s->data = NULL;
}

static struct iv2d_region const *make_difference(struct slide *s, float const *w, float const *h, float const *t) {
    struct difference_data *d = malloc(sizeof *d);
    s->data = d;
    float cx = 0.5f * *w;
    float cy = 0.5f * *h;
    float cr = 0.5f * fminf(cx, cy);
    float th = t ? *t : 0;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    d->center = (struct iv2d_circle){.region={iv2d_circle}, cx, cy, cr};
    d->orbit  = (struct iv2d_circle){.region={iv2d_circle}, ox, oy, 100};
    d->inv    = (struct iv2d_invert){.region={iv2d_invert}, &d->orbit.region};
    d->sub[0] = &d->center.region;
    d->sub[1] = &d->inv.region;
    d->op = (struct iv2d_setop){.region={iv2d_intersect}, d->sub, 2};
    return &d->op.region;
}

struct slide difference_slide = {"difference", NULL, make_difference, cleanup_difference, NULL, 0};
