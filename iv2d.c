#include "iv2d.h"

static void cover_4way_recursive(struct iv2d_rect        const  bounds,
                                 struct iv2d_edge        const *edge,
                                 struct iv2d_yield_coverage    *yield) {
    iv const e = edge->fn(edge, (iv){(float)bounds.l, (float)bounds.r}
                              , (iv){(float)bounds.t, (float)bounds.b});

    if (e.lo < 0 && e.hi < 0) {
        yield->fn(yield, bounds, 1.0f);
    }
    if (e.lo < 0 && e.hi >= 0) {
        int const x = (bounds.l+bounds.r)/2,
                  y = (bounds.t+bounds.b)/2;
        if (bounds.l == x && bounds.t == y) {
            yield->fn(yield, bounds, 0.5f/*TODO*/);
        } else {
            iv2d_cover((struct iv2d_rect){bounds.l,bounds.t, x,y}, edge, yield);
            iv2d_cover((struct iv2d_rect){bounds.l,y, x,bounds.b}, edge, yield);
            iv2d_cover((struct iv2d_rect){x,bounds.t, bounds.r,y}, edge, yield);
            iv2d_cover((struct iv2d_rect){x,y, bounds.r,bounds.b}, edge, yield);
        }
    }
}

void iv2d_cover(struct iv2d_rect        const  bounds,
                struct iv2d_edge        const *edge,
                struct iv2d_yield_coverage    *yield) {
    if (bounds.l < bounds.r && bounds.t < bounds.b) {
        cover_4way_recursive(bounds, edge, yield);
    }
}

static iv circle_edge(struct iv2d_edge const *edge, iv x, iv y) {
    struct iv2d_circle const *c = (struct iv2d_circle const*)edge;
    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c->x,c->x})),
                         iv_square(iv_sub(y, (iv){c->y,c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}
struct iv2d_circle iv2d_circle(float x, float y, float r) {
    return (struct iv2d_circle){.edge={circle_edge}, .x=x, .y=y, .r=r};
}
