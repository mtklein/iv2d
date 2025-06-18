#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

static struct iv2d_region const *make_capsule(struct slide *s, float w, float h, double t) {
    (void)s; (void)t;
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float th = (float)t;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    static struct iv2d_capsule cap;
    cap = (struct iv2d_capsule){.region={iv2d_capsule}, ox, oy, cx, cy, 4};
    return &cap.region;
}

struct slide capsule_slide = {"capsule", NULL, make_capsule, 0};
