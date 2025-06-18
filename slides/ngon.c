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

struct ngon_data {
    struct iv2d_halfplane hp[7];
    struct iv2d_region const *regions[7];
    struct iv2d_setop op;
};

static void cleanup_ngon(struct slide *s) {
    free(s->data);
    s->data = NULL;
}

static struct iv2d_region const *make_ngon(struct slide *s, float const *w, float const *h, float const *t) {
    struct ngon_data *d = malloc(sizeof *d);
    s->data = d;
    float cx = 0.5f * *w;
    float cy = 0.5f * *h;
    float th = t ? *t : 0;

    for (int i = 0; i < len(d->hp); i++) {
        double const pi = atan(1)*4;
        d->hp[i] = halfplane_from(cx + 100 * cosf(th + (float)( i   *2*pi/len(d->hp))),
                                  cy + 100 * sinf(th + (float)( i   *2*pi/len(d->hp))),
                                  cx + 100 * cosf(th + (float)((i+1)*2*pi/len(d->hp))),
                                  cy + 100 * sinf(th + (float)((i+1)*2*pi/len(d->hp))));
        d->regions[i] = &d->hp[i].region;
    }
    d->op = (struct iv2d_setop){.region={iv2d_intersect}, d->regions, len(d->hp)};
    return &d->op.region;
}

struct slide ngon_slide = {"ngon", NULL, make_ngon, cleanup_ngon, NULL, 0};
