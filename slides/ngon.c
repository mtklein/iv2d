#include "../slide.h"
#include "../iv2d_regions.h"
#include "../len.h"
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

static struct iv2d_setop intersect_halfplanes(struct iv2d_halfplane const hp[], int n, struct iv2d_region const *region[]) {
    for (int i = 0; i < n; i++) {
        region[i] = &hp[i].region;
    }
    return (struct iv2d_setop){.region={iv2d_intersect}, region, n};
}

static struct iv2d_region const *make_ngon(struct slide *s, float w, float h, double t) {
    (void)s;
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float th = (float)t;

    struct iv2d_halfplane hp[7];
    for (int i = 0; i < len(hp); i++) {
        double const pi = atan(1)*4;
        hp[i] = halfplane_from(cx + 100 * cosf(th + (float)( i   *2*pi/len(hp))),
                               cy + 100 * sinf(th + (float)( i   *2*pi/len(hp))),
                               cx + 100 * cosf(th + (float)((i+1)*2*pi/len(hp))),
                               cy + 100 * sinf(th + (float)((i+1)*2*pi/len(hp))));
    }
    static struct iv2d_region const *regions[len(hp)];
    static struct iv2d_setop op;
    op = intersect_halfplanes(hp, len(hp), regions);
    return &op.region;
}

struct slide ngon_slide = {"ngon", NULL, make_ngon, 0};
