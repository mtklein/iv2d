#include "iv2d.h"
#include <assert.h>

typedef int __attribute__((vector_size(16))) int4;

static iv lane(int i, iv4 X) {
    return (iv){X.lo[i], X.hi[i]};
}

// Our core idea is that a region function R=region(X,Y) is negative inside the region
// or positive outside it.  By convention we treat an exact 0 (on the edge) as outside.
// So all-negative means inside, all-non-negative outside, and a mix is uncertain.
static enum {INSIDE=-1,UNCERTAIN=0,OUTSIDE=+1} classify(iv R) {
    if (R.hi < 0) { return    INSIDE; }   // [-,-]
    if (R.lo < 0) { return UNCERTAIN; }   // [-,+] or [-,0]
    return OUTSIDE;                       // [+,+] or [0,+] or [0,0]
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

    int4 const inside = (LT_LB_RT_RB.hi < 0)          ,
            uncertain = (LT_LB_RT_RB.lo < 0) & ~inside;

    float cov = (float)(int)__builtin_reduce_add(inside & 1);
    if (uncertain[0]) { cov += estimate_coverage(region,ctx, lane(0,LT_LB_RT_RB), l,t,x,y, limit); }
    if (uncertain[1]) { cov += estimate_coverage(region,ctx, lane(1,LT_LB_RT_RB), l,y,x,b, limit); }
    if (uncertain[2]) { cov += estimate_coverage(region,ctx, lane(2,LT_LB_RT_RB), x,t,r,y, limit); }
    if (uncertain[3]) { cov += estimate_coverage(region,ctx, lane(3,LT_LB_RT_RB), x,y,r,b, limit); }
    return cov * 0.25f;
}

void iv2d_cover(iv2d_region *region, void const *ctx,
                int l, int t, int r, int b,
                int quality,
                void (*yield)(int,int,int,int, float, void*), void *arg) {
    if (l < r && t < b) {
        iv4 const X = { {(float)l}, {(float)r} },
                  Y = { {(float)t}, {(float)b} };
        iv  const R = lane(0, region(X,Y, ctx));

        if (classify(R) == INSIDE) {
            yield(l,t,r,b, 1.0f, arg);
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
                        yield(l,t,r,b, cov, arg);
                    }
                }
            } else {
                iv2d_cover(region,ctx, l,t,x,y, quality, yield,arg);
                iv2d_cover(region,ctx, l,y,x,b, quality, yield,arg);
                iv2d_cover(region,ctx, x,t,r,y, quality, yield,arg);
                iv2d_cover(region,ctx, x,y,r,b, quality, yield,arg);
            }
        }
    }
}

static iv4 splat(float x) {
    return (iv4){{x,x,x,x}, {x,x,x,x}};
}

iv4 iv2d_circle(iv4 X, iv4 Y, void const *ctx) {
    struct iv2d_circle const *c = ctx;

    return iv4_sub(iv4_add(iv4_square(iv4_sub(X, splat(c->x))),
                           iv4_square(iv4_sub(Y, splat(c->y)))),
                   splat(c->r * c->r));
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
