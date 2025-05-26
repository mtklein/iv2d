#include "iv2d.h"
#include <assert.h>

// TODO: cover_subpixel is only called at the top level when we already know e will be partial
//       rewrite so that's an invariant, to avoid testing edge->fn twice on the whole subpixel?
static float cover_subpixel(float l, float t, float r, float b,
                            int limit, struct iv2d_edge const *edge) {
    iv const e = edge->fn(edge, (iv){l,r}, (iv){t,b});

    if (e.lo > 0 && e.hi > 0) { return 0.0f; }
    if (e.lo < 0 && e.hi < 0) { return 1.0f; }

    if (limit <= 0) {
        return 0.5f;
    }

    float const x = (l + r) / 2,
                y = (t + b) / 2;
    return 0.25f * ( cover_subpixel(l,t, x,y, limit-1, edge)
                   + cover_subpixel(l,y, x,b, limit-1, edge)
                   + cover_subpixel(x,t, r,y, limit-1, edge)
                   + cover_subpixel(x,y, r,b, limit-1, edge) );
}

void iv2d_cover(struct iv2d_rect const   bounds,
                int              const   quality,
                struct iv2d_edge const  *edge,
                struct iv2d_coverage_cb *yield) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv const e = edge->fn(edge, (iv){(float)l, (float)r}
                                  , (iv){(float)t, (float)b});

        if (e.lo < 0 && e.hi < 0) {
            yield->fn(yield, bounds, 1.0f);
        }
        if (e.lo < 0 && e.hi >= 0) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                if (quality >= 0) {
                    float const cov = cover_subpixel((float)l, (float)t,
                                                     (float)r, (float)b,
                                                     quality, edge);
                    assert(cov > 0);  // implied by e.lo < 0 && e.hi >= 0
                    assert(cov < 1);  // would have been picked up by e.lo < 0 && e.hi < 0
                    yield->fn(yield, bounds, cov);
                }
            } else {
                iv2d_cover((struct iv2d_rect){l,t, x,y}, quality, edge, yield);
                iv2d_cover((struct iv2d_rect){l,y, x,b}, quality, edge, yield);
                iv2d_cover((struct iv2d_rect){x,t, r,y}, quality, edge, yield);
                iv2d_cover((struct iv2d_rect){x,y, r,b}, quality, edge, yield);
            }
        }
    }
}

static iv circle_edge(struct iv2d_edge const *edge, iv x, iv y) {
    struct iv2d_circle const *c = (struct iv2d_circle const*)edge;

    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c->x, c->x})),
                         iv_square(iv_sub(y, (iv){c->y, c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}
struct iv2d_circle iv2d_circle(float x, float y, float r) {
    return (struct iv2d_circle){
        .edge={circle_edge},
        .x=x, .y=y, .r=r,
    };
}
