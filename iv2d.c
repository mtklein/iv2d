#include "iv2d.h"

typedef int __attribute__((vector_size(16))) int4;

// Our core idea: a region function R=region(X,Y) is negative inside the region
// or positive outside it.  By convention we treat an exact 0 (on the edge) as outside.
// So all-negative means inside, all-non-negative outside, and a mix is uncertain.
static enum {INSIDE,OUTSIDE,UNCERTAIN} iv_classify(iv R) {
    if (R.hi < 0) { return    INSIDE; }   // [-,-]
    if (R.lo < 0) { return UNCERTAIN; }   // [-,+] or [-,0]
    return OUTSIDE;                       // [+,+] or [0,+] or [0,0]
}
static void iv4_classify(iv4 R, int4 *inside, int4 *uncertain) {
    *inside    = (R.hi < 0);
    *uncertain = (R.lo < 0) & ~*inside;
}

static float4 when(int4 mask, float4 v) {
    return (float4)( mask & (int4)v );
}

// Estimate coverage of a region bounded by {l,t,r,b} using subdivision like iv2d_cover
// but with a limit on recursion depth.
static float4 estimate_coverage(iv2d_region *region, void const *ctx,
                                float l, float t, float r, float b, int limit) {
    // Evaluate LT, LB, RT, and RB corners of the region.
    float const x = (l+r)/2,
                y = (t+b)/2;
    iv4 const corners = region((iv4){{l,l,x,x}, {x,x,r,r}},
                               (iv4){{t,y,t,y}, {y,b,y,b}}, ctx);
    int4 inside, uncertain;
    iv4_classify(corners, &inside, &uncertain);

    // Coverage of corners fully inside the region is easy, 1.0f.
    float4 cov = when(inside, (float4){1.0f,1.0f,1.0f,1.0f});

    if (--limit) {
        if (uncertain[0]) { cov += estimate_coverage(region,ctx, l,t,x,y, limit); }
        if (uncertain[1]) { cov += estimate_coverage(region,ctx, l,y,x,b, limit); }
        if (uncertain[2]) { cov += estimate_coverage(region,ctx, x,t,r,y, limit); }
        if (uncertain[3]) { cov += estimate_coverage(region,ctx, x,y,r,b, limit); }
    } else {
        // We may recurse no further, so remembering "negative inside, positive outside",
        // we estimate coverage for each uncertain corner as the proportion of its
        // region function interval that is negative.  (When uncertain, this divide is safe.)
        cov += when(uncertain, -corners.lo / (corners.hi - corners.lo));
    }
    return 0.25f * cov;
}

static iv lane(int i, iv4 X) {
    return (iv){X.lo[i], X.hi[i]};
}

void iv2d_cover(iv2d_region *region, void const *ctx,
                int l, int t, int r, int b,
                int quality,
                void (*yield)(int,int,int,int, float, void*), void *arg) {
    if (l < r && t < b) {
        // TODO: how to vectorize iv2d_cover() better?  3/4 of this call to region() is wasted.
        iv4 const X = { {(float)l}, {(float)r} },
                  Y = { {(float)t}, {(float)b} };
        iv  const R = lane(0, region(X,Y, ctx));

        if (iv_classify(R) == INSIDE) {
            yield(l,t,r,b, 1.0f, arg);
        }
        if (iv_classify(R) == UNCERTAIN) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                if (quality > 0) {
                    float4 const cov4 = estimate_coverage(region,ctx,
                                                          (float)l, (float)t, (float)r, (float)b,
                                                          quality);
                    float const cov = cov4[0] + cov4[1] + cov4[2] + cov4[3];
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
