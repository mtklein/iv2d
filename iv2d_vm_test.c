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

    iv32 x = (iv32){{0,1,2,3}, {4,5,6,7}},
         y = (iv32){{0,0,1,1}, {5,4,3,2}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_sub(x,y);
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

    iv32 x = (iv32){{1,2,3,4}, {5,6,7,8}},
         y = (iv32){{8,7,6,5}, {4,3,2,1}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_add(x,y);
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

    iv32 x = (iv32){{3,-3,-3,-3}, {4,4,4,4}},
         y = (iv32){{5,5,-5,-5}, {6,6,6,-1}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_mul(x,y);
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

static void test_div(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_div(b,x,y));
    }

    iv32 x = (iv32){{6,-6,-2,3}, {8,8,4,-3}},
         y = (iv32){{2,-2,-1,3}, {2,4,2,-1}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_div(x,y);
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

    iv32 x = (iv32){{3,-3,-3}, {4,4,4}},
         y = (iv32){{5,-5,-5}, {6,6,-1}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_min(x,y);
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

    iv32 x = (iv32){{3,-3,-3}, {4,4,4}},
         y = (iv32){{5,-5,-5}, {6,6,-1}};
    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_max(x,y);
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

    iv32 z = region->eval(region, as_iv32(0), as_iv32(0));
    iv32 e = iv32_sqrt(as_iv32(4));
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

    iv32 x = (iv32){{3,-3,-5}, {4,4,-1}};
    iv32 z = region->eval(region, x, as_iv32(0));
    iv32 e = iv32_square(x);
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

    iv32 x = (iv32){{0,-2,0}, {4,0,0}};
    iv32 z = region->eval(region, x, as_iv32(0));
    iv32 e = iv32_abs(x);
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

    iv32 x = (iv32){{+1,-4,-1,+0}, {+4,-1,+4,+0}};
    iv32 z = region->eval(region, x, as_iv32(0));
    iv32 e = iv32_inv(x);
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

    iv32 x = (iv32){{1,2,3,4}, {5,6,7,8}},
         y = (iv32){{8,7,6,5}, {4,3,2,1}};

    iv32 z = region->eval(region, x,y);
    iv32 e = iv32_mad(x,y,as_iv32(3));
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }

    u = 4;
    z = region->eval(region, x,y);
    e = iv32_mad(x,y,as_iv32(4));
    for (int i = 0; i < 4; i++) {
        expect(equiv(z.lo[i], e.lo[i]));
        expect(equiv(z.hi[i], e.hi[i]));
    }
}

int main(void) {
    test_sub();
    test_add();
    test_mul();
    test_div();
    test_min();
    test_max();
    test_sqrt();
    test_square();
    test_abs();
    test_inv();
    test_mad_imm_uni();
    return 0;
}

