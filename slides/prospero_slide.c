#pragma clang diagnostic ignored "-Wcast-qual"
#include "../slide.h"
#include "../prospero.h"

static void cleanup_prospero(struct slide *s) {
    free((void*)s->region);
    s->region = NULL;
}

static struct iv2d_region const *make_prospero(struct slide *s, float const *w, float const *h, float const *t) {
    (void)t;
    if (!s->region) {
        float W = *w, H = *h;
        s->region = prospero_region(&W, &H);
    }
    return s->region;
}

struct slide prospero_slide = {"prospero", NULL, make_prospero, cleanup_prospero, NULL, 0};
