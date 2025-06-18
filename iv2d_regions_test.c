#include "iv2d_regions.h"
#include "iv2d.h"
#include "test.h"

struct cover_ctx {
    int calls;
    float l,t,r,b,cov;
};

static void record(void *arg, float l,float t,float r,float b,float cov) {
    struct cover_ctx *ctx = arg;
    ctx->calls++;
    ctx->l=l; ctx->t=t; ctx->r=r; ctx->b=b; ctx->cov=cov;
}

static void test_circle(void) {
    struct iv2d_circle c = {.region={iv2d_circle}, 0,0,1};

    iv32 v = c.region.eval(&c.region, as_iv32(0), as_iv32(0));
    expect(equiv(v.lo[0], -1));
    expect(equiv(v.hi[0], -1));

    v = c.region.eval(&c.region, as_iv32(1.5f), as_iv32(0));
    expect(equiv(v.lo[0], 0.5f));
    expect(equiv(v.hi[0], 0.5f));
}

static void test_union(void) {
    struct iv2d_circle a = {.region={iv2d_circle}, 0,0,1};
    struct iv2d_circle b = {.region={iv2d_circle}, 2,0,1};
    struct iv2d_region const *subs[] = {&a.region,&b.region};
    struct iv2d_setop u = {.region={iv2d_union}, subs, 2};

    iv32 v = u.region.eval(&u.region, as_iv32(0), as_iv32(0));
    expect(equiv(v.lo[0], -1));
    expect(equiv(v.hi[0], -1));

    v = u.region.eval(&u.region, as_iv32(1.5f), as_iv32(0));
    expect(equiv(v.lo[0], -0.5f));
    expect(equiv(v.hi[0], -0.5f));
}

static void test_cover(void) {
    struct iv2d_circle big = {.region={iv2d_circle}, 0,0,10};
    struct cover_ctx ctx = {0};
    iv2d_cover(&big.region, 0,0,2,2, 0, record,&ctx);
    expect(ctx.calls == 1);
    expect(equiv(ctx.l,0));
    expect(equiv(ctx.t,0));
    expect(equiv(ctx.r,2));
    expect(equiv(ctx.b,2));
    expect(equiv(ctx.cov,1));

    struct iv2d_circle far = {.region={iv2d_circle}, 5,0,1};
    ctx.calls = 0;
    iv2d_cover(&far.region, 0,0,2,2, 0, record,&ctx);
    expect(ctx.calls == 0);
}

int main(void) {
    test_circle();
    test_union();
    test_cover();
    return 0;
}
