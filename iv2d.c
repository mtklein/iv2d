#include "iv2d.h"

void iv2d_cover(struct iv2d_rect bounds,
                iv (*edge)(iv x, iv y, void const *ectx), void const *ectx,
                void (*yield)(struct iv2d_rect bounds, float cov, void *yctx), void *yctx) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv const e = edge((iv){(float)l, (float)r},
                          (iv){(float)t, (float)b}, ectx);

        if (e.lo < 0 && e.hi < 0) {
            yield(bounds, 1.0f, yctx);
        }
        if (e.lo < 0 && e.hi >= 0) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                yield(bounds, 0.5f/*TODO*/, yctx);
            } else {
                iv2d_cover((struct iv2d_rect){l,t, x,y}, edge,ectx, yield,yctx);
                iv2d_cover((struct iv2d_rect){l,y, x,b}, edge,ectx, yield,yctx);
                iv2d_cover((struct iv2d_rect){x,t, r,y}, edge,ectx, yield,yctx);
                iv2d_cover((struct iv2d_rect){x,y, r,b}, edge,ectx, yield,yctx);
            }
        }
    }
}

iv iv2d_circle(iv x, iv y, void const *ctx) {
    struct iv2d_circle const *c = ctx;
    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c->x,c->x})),
                         iv_square(iv_sub(y, (iv){c->y,c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}
