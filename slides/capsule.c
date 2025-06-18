#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

struct capsule_data { struct iv2d_capsule cap; };

static void cleanup_capsule(struct slide *s) {
    free(s->data);
    s->data = NULL;
}

static struct iv2d_region const *make_capsule(struct slide *s, float const *w, float const *h, float const *t) {
    struct capsule_data *d = malloc(sizeof *d);
    s->data = d;
    float cx = 0.5f * *w;
    float cy = 0.5f * *h;
    float th = t ? *t : 0;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    d->cap = (struct iv2d_capsule){.region={iv2d_capsule}, ox, oy, cx, cy, 4};
    return &d->cap.region;
}

struct slide capsule_slide = {"capsule", NULL, make_capsule, cleanup_capsule, NULL, 0};
