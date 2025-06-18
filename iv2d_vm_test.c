#include "cleanup.h"
#include "iv2d_vm.h"
#include "test.h"
#include <stdlib.h>

static void test_sub(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_sub(b,x,y));
    }

    iv x = (iv){{0,1,2,3}, {4,5,6,7}},
       y = (iv){{0,0,1,1}, {5,4,3,2}};
    iv z = region->eval(region, x,y);
    iv e = iv_sub(x,y);
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_add(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_add(b,x,y));
    }

    iv x = (iv){{1,2,3,4}, {5,6,7,8}},
       y = (iv){{8,7,6,5}, {4,3,2,1}};
    iv z = region->eval(region, x,y);
    iv e = iv_add(x,y);
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_mul(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_mul(b,x,y));
    }

    iv x = (iv){{3,-3,-3,-3}, {4,4,4,4}},
       y = (iv){{5,5,-5,-5}, {6,6,6,-1}};
    iv z = region->eval(region, x,y);
    iv e = iv_mul(x,y);
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_min(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_min(b,x,y));
    }

    iv x = (iv){{3,-3,-3}, {4,4,4}},
       y = (iv){{5,-5,-5}, {6,6,-1}};
    iv z = region->eval(region, x,y);
    iv e = iv_min(x,y);
    for (int i = 0; i < 3; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_max(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_max(b,x,y));
    }

    iv x = (iv){{3,-3,-3}, {4,4,4}},
       y = (iv){{5,-5,-5}, {6,6,-1}};
    iv z = region->eval(region, x,y);
    iv e = iv_max(x,y);
    for (int i = 0; i < 3; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_sqrt(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const v = iv2d_imm(b, 4);
        region = iv2d_ret(b, iv2d_sqrt(b,v));
    }

    iv z = region->eval(region, as_iv(0), as_iv(0));
    iv e = iv_sqrt(as_iv(4));
    expect(equiv(z.lo[0], e.lo[0]));
    expect(equiv(z.hi[0], e.hi[0]));
}

static void test_square(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b);
        region = iv2d_ret(b, iv2d_square(b,x));
    }

    iv x = (iv){{3,-3,-5}, {4,4,-1}};
    iv z = region->eval(region, x, as_iv(0));
    iv e = iv_square(x);
    for (int i = 0; i < 3; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_abs(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b);
        region = iv2d_ret(b, iv2d_abs(b,x));
    }

    iv x = (iv){{0,-2,0}, {4,0,0}};
    iv z = region->eval(region, x, as_iv(0));
    iv e = iv_abs(x);
    for (int i = 0; i < 3; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_inv(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b);
        region = iv2d_ret(b, iv2d_inv(b,x));
    }

    iv x = (iv){{+1,-4,-1,+0}, {+4,-1,+4,+0}};
    iv z = region->eval(region, x, as_iv(0));
    iv e = iv_inv(x);
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_mad_imm_uni(void) {
    float u = 3;
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b),
                  c = iv2d_uni(b,&u);
        region = iv2d_ret(b, iv2d_mad(b,x,y,c));
    }

    iv x = (iv){{1,2,3,4}, {5,6,7,8}},
       y = (iv){{8,7,6,5}, {4,3,2,1}};
    iv z = region->eval(region, x,y);
    iv e = iv_mad(x,y,as_iv(3));
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

int main(void) {
    test_sub();
    test_add();
    test_mul();
    test_min();
    test_max();
    test_sqrt();
    test_square();
    test_abs();
    test_inv();
    test_mad_imm_uni();
    return 0;
}

