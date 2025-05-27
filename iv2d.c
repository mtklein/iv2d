#include "iv2d.h"
#include <assert.h>

// Our core idea is that a region function R=fn(X,Y) is negative inside the region or
// positive outside it.  By convention we treat an exact 0 (on the edge) as outside.
// So all-negative means inside, all-non-negative outside, and a mix is uncertain.
static enum {OUTSIDE,INSIDE,UNCERTAIN} classify(iv R) {
    if (R.hi <  0) { return  INSIDE; }   // [-,-]
    if (R.lo >= 0) { return OUTSIDE; }   // [+,+] or [0,+] or [0,0]
    return UNCERTAIN;  // R.lo < 0 ≤ R.hi,  [-,+] or [-,0]
}

// Estimate coverage of a region bounded by {l,t,r,b} that's already evaluated to R,
// using subdivision like iv2d_cover but with a limit on recursion depth.
static float estimate_coverage(float l, float t, float r, float b,
                               iv const R, struct iv2d_region const *region, int limit) {
    assert(classify(R) == UNCERTAIN);

    if (limit == 0) {
        // We can recurse no further, so remembering "negative inside, positive outside",
        // we'll estimate coverage as the proportion of the interval R that is negative.
        return -R.lo / (R.hi - R.lo);   // We know R.lo < 0 ≤ R.hi, so (R.hi - R.lo) > 0.
    }

    float const x = (l+r)/2,
                y = (t+b)/2;
    float cov = 0.0f;
    {
        iv const LT = region->fn(region, (iv){l,x}, (iv){t,y});
        if (classify(LT) == INSIDE   ) { cov += 1; }
        if (classify(LT) == UNCERTAIN) { cov += estimate_coverage(l,t,x,y, LT,region,limit-1); }
    }
    {
        iv const LB = region->fn(region, (iv){l,x}, (iv){y,b});
        if (classify(LB) == INSIDE   ) { cov += 1; }
        if (classify(LB) == UNCERTAIN) { cov += estimate_coverage(l,y,x,b, LB,region,limit-1); }
    }
    {
        iv const RT = region->fn(region, (iv){x,r}, (iv){t,y});
        if (classify(RT) == INSIDE   ) { cov += 1; }
        if (classify(RT) == UNCERTAIN) { cov += estimate_coverage(x,t,r,y, RT,region,limit-1); }
    }
    {
        iv const RB = region->fn(region, (iv){x,r}, (iv){y,b});
        if (classify(RB) == INSIDE   ) { cov += 1; }
        if (classify(RB) == UNCERTAIN) { cov += estimate_coverage(x,y,r,b, RB,region,limit-1); }
    }
    return cov * 0.25f;
}

void iv2d_cover(struct iv2d_rect   const  bounds,
                int                const  quality,
                struct iv2d_region const *region,
                struct iv2d_coverage_cb  *yield) {
    int const l = bounds.l,
              t = bounds.t,
              r = bounds.r,
              b = bounds.b;
    if (l < r && t < b) {
        iv const R = region->fn(region, (iv){(float)l, (float)r}
                                      , (iv){(float)t, (float)b});
        if (classify(R) == INSIDE) {
            yield->fn(yield, bounds, 1.0f);
        }
        if (classify(R) == UNCERTAIN) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                if (quality > 0) {
                    float const cov = estimate_coverage((float)l, (float)t, (float)r, (float)b,
                                                        R, region, quality-1);
                    if (cov > 0) {
                        yield->fn(yield, bounds, cov);
                    }
                }
            } else {
                iv2d_cover((struct iv2d_rect){l,t, x,y}, quality, region, yield);
                iv2d_cover((struct iv2d_rect){l,y, x,b}, quality, region, yield);
                iv2d_cover((struct iv2d_rect){x,t, r,y}, quality, region, yield);
                iv2d_cover((struct iv2d_rect){x,y, r,b}, quality, region, yield);
            }
        }
    }
}

static iv circle(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_circle const c = *(struct iv2d_circle const*)region;

    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c.x, c.x})),
                         iv_square(iv_sub(y, (iv){c.y, c.y}))),
                  (iv){c.r*c.r, c.r*c.r});
}

struct iv2d_circle iv2d_circle(float x, float y, float r) {
    return (struct iv2d_circle){.region.fn=circle, .x=x, .y=y, .r=r};
}
