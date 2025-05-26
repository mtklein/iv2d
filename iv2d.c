#include "iv2d.h"
#include <assert.h>

// TODO: we are testing      e.lo <  0 && e.hi <  0  for insideness
//       or should that be   e.lo <  0 && e.hi <= 0  ?
//       or maybe even       e.lo <= 0 && e.hi <= 0  ?

static float cover_subpixel(float l, float t, float r, float b,
                            iv e, int limit, struct iv2d_edge const *edge) {
    // We may assume cover_subpixel() is only called on areas with uncertain coverage.
    assert(e.lo < 0 && e.hi >= 0);

    if (limit == 0) {
        // We're at our recursion limit, so we just have to guess.
        // A basic unbiased guess is half coverage,
        #if 0
            return 0.5f;

        // But we can do a whole lot better---some might say unreasonably so---by examining e.
        // If negative edge values are inside, and positive outside, we'll guess the coverage
        // is approximately the fraction of the edge interval that is negative.
        //
        // This approximation makes quality=0 entirely acceptable, and quality=1 appear to my
        // eye almost perfect, where using 0.5 needs quality=2 or quality=3 to look good, and
        // something like quality=6 to look almost perfect.  This is _huge_.
        #else
            return -e.lo / (e.hi - e.lo);
        #endif
    }

    float const x = (l + r) / 2,
                y = (t + b) / 2;

    float cov = 0.0f;
    {
        e = edge->fn(edge, (iv){l,x}, (iv){t,y});
        if (e.lo < 0 && e.hi <  0) { cov += 1; }
        if (e.lo < 0 && e.hi >= 0) { cov += cover_subpixel(l,t,x,y, e,limit-1,edge); }
    }
    {
        e = edge->fn(edge, (iv){l,x}, (iv){y,b});
        if (e.lo < 0 && e.hi <  0) { cov += 1; }
        if (e.lo < 0 && e.hi >= 0) { cov += cover_subpixel(l,y,x,b, e,limit-1,edge); }
    }
    {
        e = edge->fn(edge, (iv){x,r}, (iv){t,y});
        if (e.lo < 0 && e.hi <  0) { cov += 1; }
        if (e.lo < 0 && e.hi >= 0) { cov += cover_subpixel(x,t,r,y, e,limit-1,edge); }
    }
    {
        e = edge->fn(edge, (iv){x,r}, (iv){y,b});
        if (e.lo < 0 && e.hi <  0) { cov += 1; }
        if (e.lo < 0 && e.hi >= 0) { cov += cover_subpixel(x,y,r,b, e,limit-1,edge); }
    }
    return cov * 0.25f;
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
                                                     e, quality, edge);
                    assert(cov >  0);  // implied by e.lo < 0 && e.hi >= 0
                    assert(cov <= 1);  // TODO: should this be cov < 1?
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
