#include "iv2d_regions.h"

// TODO: think of some nice way factor that allow fudging the SDF for fast fills
// (e.g. x^2+y^2-r^2) while still doing the proper sqrt(x^2+y^2)-r when stroking.

iv iv2d_circle(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_circle const *c = (struct iv2d_circle const*)region;
    return iv_sub(iv_sqrt(iv_add(iv_square(iv_sub(x, as_iv(c->x))),
                                 iv_square(iv_sub(y, as_iv(c->y))))),
                  as_iv(c->r));
}

iv iv2d_capsule(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_capsule const *c = (struct iv2d_capsule const*)region;

    float const dx = c->x1 - c->x0,
                dy = c->y1 - c->y0;

    iv const px = iv_sub(x, as_iv(c->x0)),
             py = iv_sub(y, as_iv(c->y0));

    iv const t = iv_mul(iv_add(iv_mul(px, as_iv(dx)),
                               iv_mul(py, as_iv(dy))),
                        as_iv(1 / (dx*dx + dy*dy)));

    iv const h = iv_max(as_iv(0), iv_min(t, as_iv(1)));

    return iv_sub(iv_sqrt(iv_add(iv_square(iv_sub(px, iv_mul(h, as_iv(dx)))),
                                 iv_square(iv_sub(py, iv_mul(h, as_iv(dy)))))),
                  as_iv(c->r));
}

iv iv2d_union(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_setop const *c = (struct iv2d_setop const*)region;
    iv v = as_iv(+1.0f/0);
    for (int i = 0; i < c->subregions; i++) {
        v = iv_min(v, c->subregion[i]->eval(c->subregion[i], x,y));
    }
    return v;
}

iv iv2d_intersect(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_setop const *c = (struct iv2d_setop const*)region;
    iv v = as_iv(-1.0f/0);
    for (int i = 0; i < c->subregions; i++) {
        v = iv_max(v, c->subregion[i]->eval(c->subregion[i], x,y));
    }
    return v;
}

iv iv2d_invert(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_invert const *c = (struct iv2d_invert const*)region;
    return iv_sub(as_iv(0), c->arg->eval(c->arg, x,y));
}

iv iv2d_stroke(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_stroke const *stroke = (struct iv2d_stroke const*)region;
    return iv_sub(iv_abs(stroke->arg->eval(stroke->arg, x,y)), as_iv(stroke->width));
}

iv iv2d_halfplane(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_halfplane const *hp = (struct iv2d_halfplane const*)region;
    return iv_sub(iv_add(iv_mul(x, as_iv(hp->nx)),
                         iv_mul(y, as_iv(hp->ny))),
                  as_iv(hp->d));
}

iv iv2d_affine(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_affine const *m = (struct iv2d_affine const*)region;
    return m->arg->eval(m->arg, iv_mad(x, as_iv(m->sx), iv_mad(y, as_iv(m->kx), as_iv(m->tx)))
                              , iv_mad(x, as_iv(m->ky), iv_mad(y, as_iv(m->sy), as_iv(m->ty))));
}
