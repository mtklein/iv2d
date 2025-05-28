#include "iv2d.h"
#include <assert.h>

// Our core idea is that a region function R=region(X,Y) is negative inside the region
// or positive outside it.  By convention we treat an exact 0 (on the edge) as outside.
// So all-negative means inside, all-non-negative outside, and a mix is uncertain.
static enum {OUTSIDE,INSIDE,UNCERTAIN} classify(iv R) {
    if (R.hi <  0) { return  INSIDE; }   // [-,-]
    if (R.lo >= 0) { return OUTSIDE; }   // [+,+] or [0,+] or [0,0]
    return UNCERTAIN;  // R.lo < 0 ≤ R.hi,  [-,+] or [-,0]
}

// Estimate coverage of a region bounded by {l,t,r,b} that's already evaluated to R,
// using subdivision like iv2d_cover but with a limit on recursion depth.
static float estimate_coverage(iv2d_region *region, void const *ctx, iv const R,
                               float l, float t, float r, float b, int limit) {
    assert(classify(R) == UNCERTAIN);

    if (--limit == 0) {
        // We can recurse no further, so remembering "negative inside, positive outside",
        // we'll estimate coverage as the proportion of the interval R that is negative.
        return -R.lo / (R.hi - R.lo);   // We know R.lo < 0 ≤ R.hi, so (R.hi - R.lo) > 0.
    }

    float const x = (l+r)/2,
                y = (t+b)/2;
    float cov = 0.0f;
    {
        iv const LT = region((iv){l,x}, (iv){t,y}, ctx);
        if (classify(LT) == INSIDE   ) { cov += 1; }
        if (classify(LT) == UNCERTAIN) { cov += estimate_coverage(region,ctx,LT, l,t,x,y,limit); }
    }
    {
        iv const LB = region((iv){l,x}, (iv){y,b}, ctx);
        if (classify(LB) == INSIDE   ) { cov += 1; }
        if (classify(LB) == UNCERTAIN) { cov += estimate_coverage(region,ctx,LB, l,y,x,b,limit); }
    }
    {
        iv const RT = region((iv){x,r}, (iv){t,y}, ctx);
        if (classify(RT) == INSIDE   ) { cov += 1; }
        if (classify(RT) == UNCERTAIN) { cov += estimate_coverage(region,ctx,RT, x,t,r,y,limit); }
    }
    {
        iv const RB = region((iv){x,r}, (iv){y,b}, ctx);
        if (classify(RB) == INSIDE   ) { cov += 1; }
        if (classify(RB) == UNCERTAIN) { cov += estimate_coverage(region,ctx,RB, x,y,r,b,limit); }
    }
    return cov * 0.25f;
}

void iv2d_cover(iv2d_region *region, void const *ctx,
                struct iv2d_rect const bounds, int const quality, struct iv2d_coverage_cb *yield) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv const R = region((iv){(float)l, (float)r},
                            (iv){(float)t, (float)b}, ctx);
        if (classify(R) == INSIDE) {
            yield->fn(yield, bounds, 1.0f);
        }
        if (classify(R) == UNCERTAIN) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                if (quality > 0) {
                    float const cov = estimate_coverage(region,ctx,R,
                                                        (float)l, (float)t, (float)r, (float)b,
                                                        quality);
                    if (cov > 0) {
                        yield->fn(yield, bounds, cov);
                    }
                }
            } else {
                iv2d_cover(region,ctx, (struct iv2d_rect){l,t, x,y}, quality, yield);
                iv2d_cover(region,ctx, (struct iv2d_rect){l,y, x,b}, quality, yield);
                iv2d_cover(region,ctx, (struct iv2d_rect){x,t, r,y}, quality, yield);
                iv2d_cover(region,ctx, (struct iv2d_rect){x,y, r,b}, quality, yield);
            }
        }
    }
}

iv iv2d_circle(iv X, iv Y, void const *ctx) {
    struct iv2d_circle const *c = ctx;

    return iv_sub(iv_add(iv_square(iv_sub(X, (iv){c->x, c->x})),
                         iv_square(iv_sub(Y, (iv){c->y, c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}

iv iv2d_union(iv X, iv Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv_min(op->a(X,Y, op->actx),
                  op->b(X,Y, op->bctx));
}
iv iv2d_intersection(iv X, iv Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv_max(op->a(X,Y, op->actx),
                  op->b(X,Y, op->bctx));
}
iv iv2d_difference(iv X, iv Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv_max(       op->a(X,Y, op->actx)  ,
                  iv_neg(op->b(X,Y, op->bctx)) );
}
