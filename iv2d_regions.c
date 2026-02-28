#include "iv2d_regions.h"
#include <math.h>

iv32 iv2d_circle(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_circle const *c = (struct iv2d_circle const*)region;

    iv32 const dx = iv32_sub(x, as_iv32(c->x)),
               dy = iv32_sub(y, as_iv32(c->y));

    iv32 const len = iv32_sqrt(iv32_add(iv32_square(dx),
                                        iv32_square(dy)));

    return iv32_sub(len, as_iv32(c->r));
}

iv32 iv2d_capsule(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_capsule const *c = (struct iv2d_capsule const*)region;

    float const dx = c->x1 - c->x0,
                dy = c->y1 - c->y0;

    iv32 const px = iv32_sub(x, as_iv32(c->x0)),
               py = iv32_sub(y, as_iv32(c->y0));

    iv32 const dot = iv32_add(iv32_mul(px, as_iv32(dx)),
                              iv32_mul(py, as_iv32(dy)));

    iv32 const t = iv32_mul(dot, as_iv32(1 / (dx*dx + dy*dy)));

    iv32 const h = iv32_max(as_iv32(0), iv32_min(t, as_iv32(1)));

    iv32 const hx = iv32_sub(px, iv32_mul(h, as_iv32(dx))),
               hy = iv32_sub(py, iv32_mul(h, as_iv32(dy)));

    iv32 const len = iv32_sqrt(iv32_add(iv32_square(hx),
                                        iv32_square(hy)));

    return iv32_sub(len, as_iv32(c->r));
}

iv32 iv2d_union(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_setop const *c = (struct iv2d_setop const*)region;
    iv32 v = as_iv32(+1.0f/0);
    for (int i = 0; i < c->subregions; i++) {
        v = iv32_min(v, c->subregion[i]->eval(c->subregion[i],x,y));
    }
    return v;
}
iv32 iv2d_intersect(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_setop const *c = (struct iv2d_setop const*)region;
    iv32 v = as_iv32(-1.0f/0);
    for (int i = 0; i < c->subregions; i++) {
        v = iv32_max(v, c->subregion[i]->eval(c->subregion[i],x,y));
    }
    return v;
}

iv32 iv2d_invert(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_invert const *c = (struct iv2d_invert const*)region;
    return iv32_sub(as_iv32(0), c->arg->eval(c->arg,x,y));
}

iv32 iv2d_stroke(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_stroke const *stroke = (struct iv2d_stroke const*)region;
    return iv32_sub(iv32_abs(stroke->arg->eval(stroke->arg,x,y)),
                  as_iv32(stroke->width));
}

iv32 iv2d_halfplane(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_halfplane const *hp = (struct iv2d_halfplane const*)region;
    return iv32_sub(iv32_add(iv32_mul(x, as_iv32(hp->nx)),
                             iv32_mul(y, as_iv32(hp->ny))),
                    as_iv32(hp->d));
}

static float bilerp(struct iv2d_sdf const *sdf, float x, float y) {
    float const fx0 = floorf(x),
                fy0 = floorf(y);
    int const x0 = (int)fx0,
              y0 = (int)fy0,
              x1 = x0+1 < sdf->w ? x0+1 : x0,
              y1 = y0+1 < sdf->h ? y0+1 : y0;
    float const tx = x - x0,
                ty = y - y0;
    float const a = (float)sdf->sdf[y0*sdf->w + x0],
                b = (float)sdf->sdf[y0*sdf->w + x1],
                c = (float)sdf->sdf[y1*sdf->w + x0],
                d = (float)sdf->sdf[y1*sdf->w + x1];
    return (1-tx) * ((1-ty)*a + ty*c)
         +    tx  * ((1-ty)*b + ty*d);
}

static float sample_sdf(struct iv2d_sdf const *sdf, float x, float y) {
    float const sx = x - sdf->x - 0.5f,
                sy = y - sdf->y - 0.5f,
                cx = fminf(fmaxf(sx, 0), sdf->w-1),
                cy = fminf(fmaxf(sy, 0), sdf->h-1),
                dx = sx - cx,
                dy = sy - cy;
    return bilerp(sdf, cx,cy) + sqrtf(dx*dx + dy*dy);
}

iv32 iv2d_sdf(struct iv2d_region const *region, iv32 x, iv32 y) {
    struct iv2d_sdf const *sdf = (struct iv2d_sdf const*)region;
    iv32 v = {0};
    for (int i = 0; i < 4; i++) {
        float const cx = 0.5f * (x.lo[i] + x.hi[i]),
                    cy = 0.5f * (y.lo[i] + y.hi[i]),
                    rx = 0.5f * (x.hi[i] - x.lo[i]),
                    ry = 0.5f * (y.hi[i] - y.lo[i]),
                    r  = sqrtf(rx*rx + ry*ry),
                    c  = sample_sdf(sdf, cx,cy);
        v.lo[i] = c - r;
        v.hi[i] = c + r;
    }
    return v;
}
