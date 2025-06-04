#include "iv2d.h"
#include <math.h>

// Our core idea: a signed distance function R=region(X,Y) is <= 0 inside the region
// or >0 outside it.  When lo < 0 < hi, we're uncertain if we're inside or outside the region.
static void classify(iv R, int4 *inside, int4 *uncertain) {
    *inside    = (R.hi <= 0);
    *uncertain = (R.lo <  0) & ~*inside;
}

// Estimate coverage of a region bounded by {l,t,r,b} with a limit on recursion depth.
static float4 estimate_coverage_(iv2d_region *region, void const *ctx,
                                 float l, float t, float r, float b, int limit) {
    // Evaluate LT, LB, RT, and RB corners of the region.
    float const x = (l+r)/2,
                y = (t+b)/2;
    iv const corners = region(ctx, (iv){{l,l,x,x}, {x,x,r,r}}
                                 , (iv){{t,y,t,y}, {y,b,y,b}});
    int4 inside, uncertain;
    classify(corners, &inside, &uncertain);

    float4 cov = when(inside, (float4){1.0f,1.0f,1.0f,1.0f});
    if (--limit) {
        if (uncertain[0]) { cov += estimate_coverage_(region,ctx, l,t,x,y, limit); }
        if (uncertain[1]) { cov += estimate_coverage_(region,ctx, l,y,x,b, limit); }
        if (uncertain[2]) { cov += estimate_coverage_(region,ctx, x,t,r,y, limit); }
        if (uncertain[3]) { cov += estimate_coverage_(region,ctx, x,y,r,b, limit); }
    } else {
        // We may recurse no further, so remembering "negative inside, positive outside",
        // we estimate coverage for each uncertain corner as the proportion of its
        // region function interval that is negative.  (When uncertain, this divide is safe.)
        cov += when(uncertain, corners.lo / (corners.lo - corners.hi));
    }
    return 0.25f * cov;
}
static float estimate_coverage(iv2d_region *region, void const *ctx,
                               float l, float t, float r, float b, int limit) {
    float4 const cov = estimate_coverage_(region,ctx, l,t,r,b, limit);
    return cov[0]+cov[1]+cov[2]+cov[3];
}

static void iv2d_cover_(iv2d_region *region, void const *ctx,
                        float l, float t, float r, float b,
                        int quality,
                        void (*yield)(void*, float,float,float,float, float), void *arg) {
    if (l<r && t<b) {
        // Evaluate LT, LB, RT, and RB corners of the region, split at integer pixels.
        float const x = floorf( (l+r)/2 ),
                    y = floorf( (t+b)/2 );
        iv const corners = region(ctx, (iv){{l,l,x,x}, {x,x,r,r}}
                                     , (iv){{t,y,t,y}, {y,b,y,b}});
        int4 inside,uncertain;
        classify(corners, &inside, &uncertain);

        if (__builtin_reduce_and(inside)) {
            yield(arg, l,t,r,b, 1);
            return;
        }

        if (inside[0] && inside[1] && l<x) { yield(arg, l,t,x,b, 1); inside[0] = inside[1] = 0; }
        if (inside[2] && inside[3] && x<r) { yield(arg, x,t,r,b, 1); inside[2] = inside[3] = 0; }
        if (inside[0] && inside[2] && t<y) { yield(arg, l,t,r,y, 1); inside[0] = inside[2] = 0; }
        if (inside[1] && inside[3] && y<b) { yield(arg, l,y,r,b, 1); inside[1] = inside[3] = 0; }

        if (__builtin_reduce_or(uncertain)) {
            if (r-l <= 1 && b-t <= 1) {  // These floats hold integers, so actually compare ==.
                if (quality > 0) {
                    float const cov = estimate_coverage(region,ctx, l,t,r,b, quality);
                    if (cov > 0) {
                        yield(arg, l,t,r,b, cov);
                    }
                }
            } else {
                int4 const not_outside = inside | uncertain;
                if (not_outside[0]) { iv2d_cover_(region,ctx, l,t,x,y, quality, yield,arg); }
                if (not_outside[1]) { iv2d_cover_(region,ctx, l,y,x,b, quality, yield,arg); }
                if (not_outside[2]) { iv2d_cover_(region,ctx, x,t,r,y, quality, yield,arg); }
                if (not_outside[3]) { iv2d_cover_(region,ctx, x,y,r,b, quality, yield,arg); }
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

iv iv2d_circle(void const *ctx, iv x, iv y) {
    struct iv2d_circle const *c = ctx;
    return iv_sub(iv_add(iv_square(iv_sub(x, as_iv(c->x))),
                         iv_square(iv_sub(y, as_iv(c->y)))),
                  as_iv(c->r * c->r));
}

_Bool iv2d_invert(struct iv2d_affine *inv, struct iv2d_affine m) {
    float const dinv = 1.0f / (m.sx*m.sy - m.kx*m.ky);
    if (!isfinite(dinv)) {
        return 0;
    }
    inv->sx = dinv *  m.sy;
    inv->sy = dinv *  m.sx;
    inv->kx = dinv * -m.kx;
    inv->ky = dinv * -m.ky;
    inv->tx = dinv * (m.kx*m.ty - m.sy*m.tx);
    inv->ty = dinv * (m.ky*m.tx - m.sx*m.ty);
    return 1;
}

iv iv2d_transform(void const *ctx, iv x, iv y) {
    struct iv2d_transform const *xform = ctx;
    struct iv2d_affine    const      m = xform->minv;
    iv X = iv_mad(x, as_iv(m.sx), iv_mad(y, as_iv(m.kx), as_iv(m.tx))),
       Y = iv_mad(x, as_iv(m.ky), iv_mad(y, as_iv(m.sy), as_iv(m.ty)));
    return xform->region(xform->ctx, X,Y);
}

iv iv2d_union(void const *ctx, iv x, iv y) {
    struct iv2d_binop const *op = ctx;
    return iv_min(op->a(op->actx, x,y),
                  op->b(op->bctx, x,y));
}
iv iv2d_intersection(void const *ctx, iv x, iv y) {
    struct iv2d_binop const *op = ctx;
    return iv_max(op->a(op->actx, x,y),
                  op->b(op->bctx, x,y));
}
iv iv2d_difference(void const *ctx, iv x, iv y) {
    struct iv2d_binop const *op = ctx;
    return iv_max(       op->a(op->actx, x,y)  ,
                  iv_neg(op->b(op->bctx, x,y)) );
}
