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

struct halfplane_data { struct iv2d_halfplane hp; };

static void cleanup_halfplane(struct slide *s) {
    free(s->data);
    s->data = NULL;
}

static struct iv2d_region const *make_halfplane(struct slide *s, float const *w, float const *h, float const *t) {
    struct halfplane_data *d = malloc(sizeof *d);
    s->data = d;
    float cx = 0.5f * *w;
    float cy = 0.5f * *h;
    float th = t ? *t : 0;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    d->hp = halfplane_from(ox, oy, cx, cy);
    return &d->hp.region;
}

struct slide halfplane_slide = {"halfplane", NULL, make_halfplane, cleanup_halfplane, NULL, 0};
