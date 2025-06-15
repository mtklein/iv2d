#include "iv2d.h"
#include <math.h>

// Our core idea: a signed distance function v=region(X,Y) is <= 0 inside the region
// or >0 outside it.  When lo < 0 < hi, we're uncertain if we're inside or outside the region.
static void classify(iv v, int4 *inside, int4 *uncertain) {
    *inside    = (v.hi <= 0);
    *uncertain = (v.lo <  0) & ~*inside;
}

// Estimate coverage of a region bounded by {l,t,r,b} with a limit on recursion depth.
static float4 estimate_coverage_(struct iv2d_region const *region,
                                 float l, float t, float r, float b, int limit) {
    // Evaluate LT, LB, RT, and RB corners of the region.
    float const x = (l+r)/2,
                y = (t+b)/2;
    iv const corners = region->eval(region, (iv){{l,l,x,x}, {x,x,r,r}}
                                          , (iv){{t,y,t,y}, {y,b,y,b}});
    int4 inside, uncertain;
    classify(corners, &inside, &uncertain);

    float4 cov = when(inside, (float4){1.0f,1.0f,1.0f,1.0f});
    if (--limit) {
        if (uncertain[0]) { cov += estimate_coverage_(region, l,t,x,y, limit); }
        if (uncertain[1]) { cov += estimate_coverage_(region, l,y,x,b, limit); }
        if (uncertain[2]) { cov += estimate_coverage_(region, x,t,r,y, limit); }
        if (uncertain[3]) { cov += estimate_coverage_(region, x,y,r,b, limit); }
    } else {
        // We may recurse no further, so remembering "negative inside, positive outside",
        // we estimate coverage for each uncertain corner as the proportion of its
        // region function interval that is negative.  (When uncertain, this divide is safe.)
        cov += when(uncertain, corners.lo / (corners.lo - corners.hi));
    }
    return 0.25f * cov;
}
static float estimate_coverage(struct iv2d_region const *region,
                               float l, float t, float r, float b, int limit) {
    float4 const cov = estimate_coverage_(region, l,t,r,b, limit);
    return cov[0]+cov[1]+cov[2]+cov[3];
}

static void iv2d_cover_(struct iv2d_region const *region,
                        float l, float t, float r, float b, int quality,
                        void (*yield)(void*, float,float,float,float, float), void *ctx) {
    if (l<r && t<b) {
        // Evaluate LT, LB, RT, and RB corners of the region, split at integer pixels.
        float const x = floorf( (l+r)/2 ),
                    y = floorf( (t+b)/2 );
        iv const corners = region->eval(region, (iv){{l,l,x,x}, {x,x,r,r}}
                                              , (iv){{t,y,t,y}, {y,b,y,b}});
        int4 inside,uncertain;
        classify(corners, &inside, &uncertain);

        if (__builtin_reduce_and(inside)) {
            yield(ctx, l,t,r,b, 1);
            return;
        }

        if (inside[0] && inside[1] && l<x) { yield(ctx, l,t,x,b, 1); inside[0] = inside[1] = 0; }
        if (inside[2] && inside[3] && x<r) { yield(ctx, x,t,r,b, 1); inside[2] = inside[3] = 0; }
        if (inside[0] && inside[2] && t<y) { yield(ctx, l,t,r,y, 1); inside[0] = inside[2] = 0; }
        if (inside[1] && inside[3] && y<b) { yield(ctx, l,y,r,b, 1); inside[1] = inside[3] = 0; }

        if (inside[0] && l<x && t<y) { yield(ctx, l,t,x,y, 1); }
        if (inside[1] && l<x && y<b) { yield(ctx, l,y,x,b, 1); }
        if (inside[2] && x<r && t<y) { yield(ctx, x,t,r,y, 1); }
        if (inside[3] && x<r && y<b) { yield(ctx, x,y,r,b, 1); }

        if (__builtin_reduce_or(uncertain)) {
            if (r-l <= 1 && b-t <= 1) {  // These floats hold integers, so actually compare ==.
                if (quality > 0) {
                    float const cov = estimate_coverage(region, l,t,r,b, quality);
                    if (cov > 0) {
                        yield(ctx, l,t,r,b, cov);
                    }
                }
            } else {
                if (uncertain[0]) { iv2d_cover_(region, l,t,x,y, quality, yield,ctx); }
                if (uncertain[1]) { iv2d_cover_(region, l,y,x,b, quality, yield,ctx); }
                if (uncertain[2]) { iv2d_cover_(region, x,t,r,y, quality, yield,ctx); }
                if (uncertain[3]) { iv2d_cover_(region, x,y,r,b, quality, yield,ctx); }
            }
        }
    }
}
void iv2d_cover(struct iv2d_region const *region,
                int l, int t, int r, int b,
                int quality,
                void (*yield)(void*, float,float,float,float, float), void *ctx) {
    iv2d_cover_(region, (float)l, (float)t, (float)r, (float)b, quality, yield,ctx);
}
