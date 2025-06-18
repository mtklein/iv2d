#include "../slide.h"
#include "../iv2d_regions.h"
#include <math.h>

static struct iv2d_halfplane halfplane_from(float x0,float y0,float x1,float y1){
    float dx = x1 - x0;
    float dy = y1 - y0;
    float norm = 1.0f / sqrtf(dx*dx + dy*dy);
    float nx = +dy*norm;
    float ny = -dx*norm;
    float d = nx*x0 + ny*y0;
    return (struct iv2d_halfplane){.region={iv2d_halfplane}, nx,ny,d};
}

static struct iv2d_region const *make_halfplane(struct slide *s, float w, float h, double t) {
    (void)s; (void)t;
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float th = (float)t;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    static struct iv2d_halfplane hp;
    hp = halfplane_from(ox, oy, cx, cy);
    return &hp.region;
}

struct slide halfplane_slide = {"halfplane", NULL, make_halfplane, 0};
