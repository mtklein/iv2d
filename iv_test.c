#include "iv.h"
#include "test.h"

static void test_add(void) {
    iv32 z = iv32_add((iv32){{3},{4}}, (iv32){{5},{6}});
    expect(equiv(z.lo[0],  8));
    expect(equiv(z.hi[0], 10));
}
static void test_add16(void) {
    iv16 z = iv16_add((iv16){{3},{4}}, (iv16){{5},{6}});
    expect(equiv(z.lo[0],  8));
    expect(equiv(z.hi[0], 10));
}


static void test_sub(void) {
    iv32 z = iv32_sub((iv32){{3},{4}}, (iv32){{5},{6}});
    expect(equiv(z.lo[0], -3));
    expect(equiv(z.hi[0], -1));
}
static void test_sub16(void) {
    iv16 z = iv16_sub((iv16){{3},{4}}, (iv16){{5},{6}});
    expect(equiv(z.lo[0], -3));
    expect(equiv(z.hi[0], -1));
}


static void test_mul(void) {
    iv32 z = iv32_mul((iv32){{3,-3,-3,-3},{4,4,4, 4}},
                  (iv32){{5, 5,-5,-5},{6,6,6,-1}});
    expect(equiv(z.lo[0],  15));
    expect(equiv(z.hi[0],  24));

    expect(equiv(z.lo[1], -18));
    expect(equiv(z.hi[1],  24));

    expect(equiv(z.lo[2], -20));
    expect(equiv(z.hi[2],  24));

    expect(equiv(z.lo[3], -20));
    expect(equiv(z.hi[3],  15));
}
static void test_mul16(void) {
    iv16 z = iv16_mul((iv16){{3,-3,-3,-3},{4,4,4, 4}},
                      (iv16){{5, 5,-5,-5},{6,6,6,-1}});
    expect(equiv(z.lo[0],  15));
    expect(equiv(z.hi[0],  24));

    expect(equiv(z.lo[1], -18));
    expect(equiv(z.hi[1],  24));

    expect(equiv(z.lo[2], -20));
    expect(equiv(z.hi[2],  24));

    expect(equiv(z.lo[3], -20));
    expect(equiv(z.hi[3],  15));
}


static void test_mad(void) {
    iv32 z = iv32_mad((iv32){{1,-1,-2,-1},{2,2,2,1}},
                  (iv32){{2,-3,-1,0},{3,3,1,0}},
                  (iv32){{1,1,0,0},{1,1,0,0}});
    expect(equiv(z.lo[0],  3));
    expect(equiv(z.hi[0],  7));

    expect(equiv(z.lo[1], -5));
    expect(equiv(z.hi[1],  7));

    expect(equiv(z.lo[2], -2));
    expect(equiv(z.hi[2],  2));

    expect(equiv(z.lo[3],  0));
    expect(equiv(z.hi[3],  0));
}
static void test_mad16(void) {
    iv16 z = iv16_mad((iv16){{1,-1,-2,-1},{2,2,2,1}},
                      (iv16){{2,-3,-1,0},{3,3,1,0}},
                      (iv16){{1,1,0,0},{1,1,0,0}});
    expect(equiv(z.lo[0],  3));
    expect(equiv(z.hi[0],  7));

    expect(equiv(z.lo[1], -5));
    expect(equiv(z.hi[1],  7));

    expect(equiv(z.lo[2], -2));
    expect(equiv(z.hi[2],  2));

    expect(equiv(z.lo[3],  0));
    expect(equiv(z.hi[3],  0));
}


static void test_min(void) {
    iv32 z = iv32_min((iv32){{3,-3,-3},{4,4, 4}},
                  (iv32){{5,-5,-5},{6,6,-1}});
    expect(equiv(z.lo[0],  3));
    expect(equiv(z.hi[0],  4));

    expect(equiv(z.lo[1], -5));
    expect(equiv(z.hi[1],  4));

    expect(equiv(z.lo[2], -5));
    expect(equiv(z.hi[2], -1));
}
static void test_min16(void) {
    iv16 z = iv16_min((iv16){{3,-3,-3},{4,4, 4}},
                      (iv16){{5,-5,-5},{6,6,-1}});
    expect(equiv(z.lo[0],  3));
    expect(equiv(z.hi[0],  4));

    expect(equiv(z.lo[1], -5));
    expect(equiv(z.hi[1],  4));

    expect(equiv(z.lo[2], -5));
    expect(equiv(z.hi[2], -1));
}


static void test_max(void) {
    iv32 z = iv32_max((iv32){{3,-3,-3},{4,4, 4}},
                  (iv32){{5,-5,-5},{6,6,-1}});
    expect(equiv(z.lo[0],  5));
    expect(equiv(z.hi[0],  6));

    expect(equiv(z.lo[1], -3));
    expect(equiv(z.hi[1],  6));

    expect(equiv(z.lo[2], -3));
    expect(equiv(z.hi[2],  4));
}
static void test_max16(void) {
    iv16 z = iv16_max((iv16){{3,-3,-3},{4,4, 4}},
                      (iv16){{5,-5,-5},{6,6,-1}});
    expect(equiv(z.lo[0],  5));
    expect(equiv(z.hi[0],  6));

    expect(equiv(z.lo[1], -3));
    expect(equiv(z.hi[1],  6));

    expect(equiv(z.lo[2], -3));
    expect(equiv(z.hi[2],  4));
}


static void test_sqrt(void) {
    iv32 z = iv32_sqrt((iv32){{4},{16}});
    expect(equiv(z.lo[0], 2));
    expect(equiv(z.hi[0], 4));
}
static void test_sqrt16(void) {
    iv16 z = iv16_sqrt((iv16){{4},{16}});
    expect(equiv(z.lo[0], 2));
    expect(equiv(z.hi[0], 4));
}


static void test_square(void) {
    {
        iv32 z = iv32_square((iv32){{3,-3,-5},{4,4,-1}});
        expect(equiv(z.lo[0],  9));
        expect(equiv(z.hi[0], 16));

        expect(equiv(z.lo[1],  0));
        expect(equiv(z.hi[1], 16));

        expect(equiv(z.lo[2],  1));
        expect(equiv(z.hi[2], 25));
    }

    {
        iv32 z = iv32_square((iv32){{0,-2,0},{4,0,0}});
        expect(equiv(z.lo[0],  0));
        expect(equiv(z.hi[0], 16));

        expect(equiv(z.lo[1],  0));
        expect(equiv(z.hi[1],  4));

        expect(equiv(z.lo[2],  0));
        expect(equiv(z.hi[2],  0));
    }
}
static void test_square16(void) {
    {
        iv16 z = iv16_square((iv16){{3,-3,-5},{4,4,-1}});
        expect(equiv(z.lo[0],  9));
        expect(equiv(z.hi[0], 16));

        expect(equiv(z.lo[1],  0));
        expect(equiv(z.hi[1], 16));

        expect(equiv(z.lo[2],  1));
        expect(equiv(z.hi[2], 25));
    }

    {
        iv16 z = iv16_square((iv16){{0,-2,0},{4,0,0}});
        expect(equiv(z.lo[0],  0));
        expect(equiv(z.hi[0], 16));

        expect(equiv(z.lo[1],  0));
        expect(equiv(z.hi[1],  4));

        expect(equiv(z.lo[2],  0));
        expect(equiv(z.hi[2],  0));
    }
}


static void test_abs(void) {
    {
        iv32 z = iv32_abs((iv32){{3,-3,-5},{4,4,-1}});
        expect(equiv(z.lo[0], 3));
        expect(equiv(z.hi[0], 4));

        expect(equiv(z.lo[1], 0));
        expect(equiv(z.hi[1], 4));

        expect(equiv(z.lo[2], 1));
        expect(equiv(z.hi[2], 5));
    }

    {
        iv32 z = iv32_abs((iv32){{0,-2,0},{4,0,0}});
        expect(equiv(z.lo[0], 0));
        expect(equiv(z.hi[0], 4));

        expect(equiv(z.lo[1], 0));
        expect(equiv(z.hi[1], 2));

        expect(equiv(z.lo[2], 0));
        expect(equiv(z.hi[2], 0));
    }
}
static void test_abs16(void) {
    {
        iv16 z = iv16_abs((iv16){{3,-3,-5},{4,4,-1}});
        expect(equiv(z.lo[0], 3));
        expect(equiv(z.hi[0], 4));

        expect(equiv(z.lo[1], 0));
        expect(equiv(z.hi[1], 4));

        expect(equiv(z.lo[2], 1));
        expect(equiv(z.hi[2], 5));
    }

    {
        iv16 z = iv16_abs((iv16){{0,-2,0},{4,0,0}});
        expect(equiv(z.lo[0], 0));
        expect(equiv(z.hi[0], 4));

        expect(equiv(z.lo[1], 0));
        expect(equiv(z.hi[1], 2));

        expect(equiv(z.lo[2], 0));
        expect(equiv(z.hi[2], 0));
    }
}


static void test_inv(void) {
    {
        iv32 z = iv32_inv((iv32){{+1,-4,-1,+0},
                           {+4,-1,+4,+0}});
        expect(equiv(z.lo[0], 0.25));
        expect(equiv(z.hi[0], 1   ));

        expect(equiv(z.lo[1], -1.00));
        expect(equiv(z.hi[1], -0.25));

        expect(equiv(z.lo[2], -1/0.0));
        expect(equiv(z.hi[2], +1/0.0));

        expect(equiv(z.lo[3], -1/0.0));
        expect(equiv(z.hi[3], +1/0.0));
    }
    {
        iv32 z = iv32_inv((iv32){{-0,-0,-1, 0},
                           {-0,+0, 0,+4}});
        expect(equiv(z.lo[0], -1/0.0));
        expect(equiv(z.hi[0], +1/0.0));

        expect(equiv(z.lo[1], -1/0.0));
        expect(equiv(z.hi[1], +1/0.0));

        expect(equiv(z.lo[2], -1/0.0));
        expect(equiv(z.hi[2], -1    ));

        expect(equiv(z.lo[3],  0.25 ));
        expect(equiv(z.hi[3], +1/0.0));
    }
}
static void test_inv16(void) {
    {
        iv16 z = iv16_inv((iv16){{+1,-4,-1,+0},
                                 {+4,-1,+4,+0}});
        expect(equiv(z.lo[0], 0.25));
        expect(equiv(z.hi[0], 1   ));

        expect(equiv(z.lo[1], -1.00));
        expect(equiv(z.hi[1], -0.25));

        expect(equiv(z.lo[2], -1/0.0));
        expect(equiv(z.hi[2], +1/0.0));

        expect(equiv(z.lo[3], -1/0.0));
        expect(equiv(z.hi[3], +1/0.0));
    }
    {
        iv16 z = iv16_inv((iv16){{-0,-0,-1, 0},
                                 {-0,+0, 0,+4}});
        expect(equiv(z.lo[0], -1/0.0));
        expect(equiv(z.hi[0], +1/0.0));

        expect(equiv(z.lo[1], -1/0.0));
        expect(equiv(z.hi[1], +1/0.0));

        expect(equiv(z.lo[2], -1/0.0));
        expect(equiv(z.hi[2], -1    ));

        expect(equiv(z.lo[3],  0.25 ));
        expect(equiv(z.hi[3], +1/0.0));
    }
}


int main(void) {
    test_add();    test_add16();
    test_sub();    test_sub16();
    test_mul();    test_mul16();
    test_mad();    test_mad16();
    test_min();    test_min16();
    test_max();    test_max16();
    test_sqrt();   test_sqrt16();
    test_square(); test_square16();
    test_abs();    test_abs16();
    test_inv();    test_inv16();
    return 0;
}
