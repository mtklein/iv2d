#include "iv2d.h"

static void cover_4way_recursive(struct iv2d_rect bounds,
                                 struct iv2d_edge edge,
                                 struct iv2d_cover_yield yield) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv const e = edge.fn((iv){(float)l, (float)r},
                             (iv){(float)t, (float)b}, edge.ctx);

        if (e.lo < 0 && e.hi < 0) {
            yield.fn(bounds, 1.0f, yield.ctx);
        }
        if (e.lo < 0 && e.hi >= 0) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                yield.fn(bounds, 0.5f/*TODO*/, yield.ctx);
            } else {
                iv2d_cover((struct iv2d_rect){l,t, x,y}, edge, yield);
                iv2d_cover((struct iv2d_rect){l,y, x,b}, edge, yield);
                iv2d_cover((struct iv2d_rect){x,t, r,y}, edge, yield);
                iv2d_cover((struct iv2d_rect){x,y, r,b}, edge, yield);
            }
        }
    }
}

void iv2d_cover(struct iv2d_rect bounds,
                struct iv2d_edge edge,
                struct iv2d_cover_yield yield) {
    cover_4way_recursive(bounds, edge, yield);
}

iv iv2d_circle(iv x, iv y, void const *ctx) {
    struct iv2d_circle const *c = ctx;
    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c->x,c->x})),
                         iv_square(iv_sub(y, (iv){c->y,c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}
