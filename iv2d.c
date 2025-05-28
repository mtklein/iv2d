#include "iv2d.h"
#include <assert.h>

static iv lane(int i, iv4 X) {
    return (iv){X.lo[i], X.hi[i]};
}

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
    iv4 const X = {{l,l,x,x}, {x,x,r,r}},
              Y = {{t,y,t,y}, {y,b,y,b}};

    iv4 const LT_LB_RT_RB = region(X,Y, ctx);
    float cov = 0.0f;
    {
        iv const LT = lane(0, LT_LB_RT_RB);
        if (classify(LT) == INSIDE   ) { cov += 1; }
        if (classify(LT) == UNCERTAIN) { cov += estimate_coverage(region,ctx,LT, l,t,x,y,limit); }
    }
    {
        iv const LB = lane(1, LT_LB_RT_RB);
        if (classify(LB) == INSIDE   ) { cov += 1; }
        if (classify(LB) == UNCERTAIN) { cov += estimate_coverage(region,ctx,LB, l,y,x,b,limit); }
    }
    {
        iv const RT = lane(2, LT_LB_RT_RB);
        if (classify(RT) == INSIDE   ) { cov += 1; }
        if (classify(RT) == UNCERTAIN) { cov += estimate_coverage(region,ctx,RT, x,t,r,y,limit); }
    }
    {
        iv const RB = lane(3, LT_LB_RT_RB);
        if (classify(RB) == INSIDE   ) { cov += 1; }
        if (classify(RB) == UNCERTAIN) { cov += estimate_coverage(region,ctx,RB, x,y,r,b,limit); }
    }
    return cov * 0.25f;
}

void iv2d_cover(iv2d_region *region, void const *ctx,
                struct iv2d_rect const bounds, int const quality,
                void (*yield)(struct iv2d_rect, float, void*), void *yield_ctx) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv4 const X = (iv4){{(float)l}, {(float)r}},
                  Y = (iv4){{(float)t}, {(float)b}};
        iv  const R = lane(0, region(X,Y, ctx));

        if (classify(R) == INSIDE) {
            yield(bounds, 1.0f, yield_ctx);
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
                        yield(bounds, cov, yield_ctx);
                    }
                }
            } else {
                iv2d_cover(region,ctx, (struct iv2d_rect){l,t, x,y}, quality, yield,yield_ctx);
                iv2d_cover(region,ctx, (struct iv2d_rect){l,y, x,b}, quality, yield,yield_ctx);
                iv2d_cover(region,ctx, (struct iv2d_rect){x,t, r,y}, quality, yield,yield_ctx);
                iv2d_cover(region,ctx, (struct iv2d_rect){x,y, r,b}, quality, yield,yield_ctx);
            }
        }
    }
}

static iv4 iv4_(float x) {
    return (iv4){{x,x,x,x}, {x,x,x,x}};
}

iv4 iv2d_circle(iv4 X, iv4 Y, void const *ctx) {
    struct iv2d_circle const *c = ctx;

    return iv4_sub(iv4_add(iv4_square(iv4_sub(X, iv4_(c->x))),
                           iv4_square(iv4_sub(Y, iv4_(c->y)))),
                   iv4_(c->r * c->r));
}

iv4 iv2d_union(iv4 X, iv4 Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv4_min(op->a(X,Y, op->actx),
                   op->b(X,Y, op->bctx));
}
iv4 iv2d_intersection(iv4 X, iv4 Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv4_max(op->a(X,Y, op->actx),
                   op->b(X,Y, op->bctx));
}
iv4 iv2d_difference(iv4 X, iv4 Y, void const *ctx) {
    struct iv2d_binop const *op = ctx;
    return iv4_max(        op->a(X,Y, op->actx)  ,
                   iv4_neg(op->b(X,Y, op->bctx)) );
}
