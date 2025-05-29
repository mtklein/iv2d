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

// Estimate coverage of a region bounded by {l,t,r,b} with a limit on recursion depth.
static float4 estimate_coverage(iv2d_region *region, void const *ctx,
                                float l, float t, float r, float b, int limit) {
    // Evaluate LT, LB, RT, and RB corners of the region.
    float const x = (l+r)/2,
                y = (t+b)/2;
    iv4 const corners = region(ctx, (iv4){{l,l,x,x}, {x,x,r,r}}
                                  , (iv4){{t,y,t,y}, {y,b,y,b}});
    int4 inside, uncertain;
    iv4_classify(corners, &inside, &uncertain);

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
        cov += when(uncertain, corners.lo / (corners.lo - corners.hi));
    }
    return 0.25f * cov;
}

static iv lane(int i, iv4 X) {
    return (iv){X.lo[i], X.hi[i]};
}

static void iv2d_cover_(iv2d_region *region, void const *ctx,
                        float l, float t, float r, float b,
                        int quality,
                        void (*yield)(void*, float,float,float,float, float), void *arg) {
    if (l < r && t < b) {
        // TODO: how to vectorize better here?  3/4 of this call to region() is wasted.
        iv  const R = lane(0, region(ctx, (iv4){{l},{r}}
                                        , (iv4){{t},{b}}));

        if (iv_classify(R) == INSIDE) {
            yield(arg, l,t,r,b, 1.0f);
        }
        if (iv_classify(R) == UNCERTAIN) {
            if (r-l <= 1 && b-t <= 1) {
                if (quality > 0) {
                    float4 const cov4 = estimate_coverage(region,ctx, l,t,r,b, quality);
                    float  const cov  = cov4[0] + cov4[1] + cov4[2] + cov4[3];
                    if (cov > 0) {
                        yield(arg, l,t,r,b, cov);
                    }
                }
            } else {
                float const x = floorf( (l+r)/2 ),
                            y = floorf( (t+b)/2 );
                iv2d_cover_(region,ctx, l,t,x,y, quality, yield,arg);
                iv2d_cover_(region,ctx, l,y,x,b, quality, yield,arg);
                iv2d_cover_(region,ctx, x,t,r,y, quality, yield,arg);
                iv2d_cover_(region,ctx, x,y,r,b, quality, yield,arg);
            }
        }
    }
}
void iv2d_cover(iv2d_region *region, void const *ctx,
                int l, int t, int r, int b,
                int quality,
                void (*yield)(void*, float,float,float,float, float), void *arg) {
    iv2d_cover_(region,ctx, (float)l, (float)t, (float)r, (float)b, quality, yield,arg);
}

static iv4 splat(float x) {
    return (iv4){{x,x,x,x}, {x,x,x,x}};
}

iv4 iv2d_circle(void const *ctx, iv4 X, iv4 Y) {
    struct iv2d_circle const *c = ctx;
    return iv4_sub(iv4_add(iv4_square(iv4_sub(X, splat(c->x))),
                           iv4_square(iv4_sub(Y, splat(c->y)))),
                   splat(c->r * c->r));
}

iv4 iv2d_union(void const *ctx, iv4 X, iv4 Y) {
    struct iv2d_binop const *op = ctx;
    return iv4_min(op->a(op->actx, X,Y),
                   op->b(op->bctx, X,Y));
}
iv4 iv2d_intersection(void const *ctx, iv4 X, iv4 Y) {
    struct iv2d_binop const *op = ctx;
    return iv4_max(op->a(op->actx, X,Y),
                   op->b(op->bctx, X,Y));
}
iv4 iv2d_difference(void const *ctx, iv4 X, iv4 Y) {
    struct iv2d_binop const *op = ctx;
    return iv4_max(        op->a(op->actx, X,Y)  ,
                   iv4_neg(op->b(op->bctx, X,Y)) );
}
