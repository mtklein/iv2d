#include "iv2d.h"
#include <assert.h>

// Transform the result of evaluating an iv2d_region into an inside/outside coverage decision.
// Remember, the general idea is that the expression is negative inside the region.
// I'm not exactly sure how I want to treat exact 0, or even perhaps if ±0 should be treated
// differently, but for now I try to consistently treat 0 as non-negative.
static enum {NONE,FULL,MAYBE} coverage(iv edge) {
    assert(edge.lo <= edge.hi);
    if (edge.hi <  0) { return FULL; }    // [-a, -b]                think, "all negative"
    if (edge.lo >= 0) { return NONE; }    // [+a, +b] or [ 0, +b] or [0,0]  "non-negative"
    assert(edge.lo < 0 && 0 <= edge.hi);  // [-a, +b] or [-a,  0]           "some negative"
    return MAYBE;
}

static float cover_subpixel(float l, float t, float r, float b,
                            iv const edge, int limit, struct iv2d_region const *region) {
    // We may assume cover_subpixel() is only called on areas with uncertain coverage.
    assert(coverage(edge) == MAYBE);

    if (limit == 0) {
        // We're at our recursion limit, so we just have to guess.
        // A basic unbiased guess is half coverage,
        #if 0
            return 0.5f;

        // But we can do a whole lot better---some might say unreasonably so---by examining edge.
        // If negative values are inside a region, and positive outside, we'll guess the coverage
        // is approximately the fraction of the interval that is negative.
        //
        // This approximation makes quality=1 entirely acceptable, and quality=2 appear to my
        // eye almost perfect, where using 0.5 needs quality=3 or quality=4 to look good, and
        // something like quality=7 to look almost perfect.  This is _huge_.
        #else
            // coverage(edge) == MAYBE implies edge.lo < 0 <= edge.hi, so
            //   1) edge.lo           is non-zero
            //   2) edge.hi - edge.lo is non-zero
            // which together mean the divide is safe and produces a non-zero coverage estimate.
            assert(edge.lo           < 0);
            assert(edge.hi - edge.lo > 0);
            return -edge.lo / (edge.hi - edge.lo);
        #endif
    }

    float const x = (l + r) / 2,
                y = (t + b) / 2;

    float cov = 0.0f;
    {
        iv const lt = region->fn(region, (iv){l,x}, (iv){t,y});
        if (coverage(lt) == FULL ) { cov += 1; }
        if (coverage(lt) == MAYBE) { cov += cover_subpixel(l,t,x,y, lt,limit-1,region); }
    }
    {
        iv const lb = region->fn(region, (iv){l,x}, (iv){y,b});
        if (coverage(lb) == FULL ) { cov += 1; }
        if (coverage(lb) == MAYBE) { cov += cover_subpixel(l,y,x,b, lb,limit-1,region); }
    }
    {
        iv const rt = region->fn(region, (iv){x,r}, (iv){t,y});
        if (coverage(rt) == FULL ) { cov += 1; }
        if (coverage(rt) == MAYBE) { cov += cover_subpixel(x,t,r,y, rt,limit-1,region); }
    }
    {
        iv const rb = region->fn(region, (iv){x,r}, (iv){y,b});
        if (coverage(rb) == FULL ) { cov += 1; }
        if (coverage(rb) == MAYBE) { cov += cover_subpixel(x,y,r,b, rb,limit-1,region); }
    }
    // If the whole box has MAYBE coverage, at least one of its corners will give us _something_.
    assert(cov > 0);
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
        iv const edge = region->fn(region, (iv){(float)l, (float)r}
                                         , (iv){(float)t, (float)b});

        if (coverage(edge) == FULL) {
            yield->fn(yield, bounds, 1.0f);
        }
        if (coverage(edge) == MAYBE) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                if (quality > 0) {
                    float const cov = cover_subpixel((float)l, (float)t,
                                                     (float)r, (float)b,
                                                     edge, quality-1, region);
                    assert(cov > 0);
                    yield->fn(yield, bounds, cov);
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
    struct iv2d_circle const *c = (struct iv2d_circle const*)region;

    return iv_sub(iv_add(iv_square(iv_sub(x, (iv){c->x, c->x})),
                         iv_square(iv_sub(y, (iv){c->y, c->y}))),
                  (iv){c->r*c->r, c->r*c->r});
}
struct iv2d_circle iv2d_circle(float x, float y, float r) {
    return (struct iv2d_circle){
        .region={circle},
        .x=x, .y=y, .r=r,
    };
}
