#include "../slide.h"
#include "../prospero.h"

static struct iv2d_region const *make_prospero(struct slide *s, float w, float h, double t) {
    (void)t;
    if (!s->region) {
        float W = w, H = h;
        s->region = prospero_region(&W, &H);
    }
    return s->region;
}

struct slide prospero_slide = {"prospero", NULL, make_prospero, 1};
