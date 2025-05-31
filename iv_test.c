#include "iv.h"
#include <stdio.h>

#define expect(x) if (!(x)) dprintf(2, "%s:%d %s expect(%s)\n", __FILE__, __LINE__, __func__, #x), \
                            __builtin_debugtrap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

static void test_add(void) {
    iv Z = iv_add((iv){3,4}, (iv){5,6});
    expect(equiv(Z.lo,  8));
    expect(equiv(Z.hi, 10));
}

static void test_sub(void) {
    iv Z = iv_sub((iv){3,4}, (iv){5,6});
    expect(equiv(Z.lo, -3));
    expect(equiv(Z.hi, -1));
}

static void test_mul(void) {
    {
        iv Z = iv_mul((iv){3,4}, (iv){5,6});
        expect(equiv(Z.lo, 15));
        expect(equiv(Z.hi, 24));
    }
    {
        iv Z = iv_mul((iv){-3,4}, (iv){5,6});
        expect(equiv(Z.lo, -18));
        expect(equiv(Z.hi,  24));
    }
    {
        iv Z = iv_mul((iv){-3,4}, (iv){-5,6});
        expect(equiv(Z.lo, -20));
        expect(equiv(Z.hi,  24));
    }
    {
        iv Z = iv_mul((iv){-3,4}, (iv){-5,-1});
        expect(equiv(Z.lo, -20));
        expect(equiv(Z.hi,  15));
    }
}

static void test_mul4(void) {
    {
        iv4 Z = iv4_mul((iv4){{3,-3,-3,-3},{4,4,4, 4}},
                        (iv4){{5, 5,-5,-5},{6,6,6,-1}});
        expect(equiv(Z.lo[0],  15));
        expect(equiv(Z.hi[0],  24));
        expect(equiv(Z.lo[1], -18));
        expect(equiv(Z.hi[1],  24));
        expect(equiv(Z.lo[2], -20));
        expect(equiv(Z.hi[2],  24));
        expect(equiv(Z.lo[3], -20));
        expect(equiv(Z.hi[3],  15));
    }
}

static void test_min(void) {
    {
        iv Z = iv_min((iv){3,4}, (iv){5,6});
        expect(equiv(Z.lo, 3));
        expect(equiv(Z.hi, 4));
    }
    {
        iv Z = iv_min((iv){-3,4}, (iv){-5,6});
        expect(equiv(Z.lo, -5));
        expect(equiv(Z.hi,  4));
    }
    {
        iv Z = iv_min((iv){-3,4}, (iv){-5,-1});
        expect(equiv(Z.lo, -5));
        expect(equiv(Z.hi, -1));
    }
}

static void test_max(void) {
    {
        iv Z = iv_max((iv){3,4}, (iv){5,6});
        expect(equiv(Z.lo, 5));
        expect(equiv(Z.hi, 6));
    }
    {
        iv Z = iv_max((iv){-3,4}, (iv){-5,6});
        expect(equiv(Z.lo, -3));
        expect(equiv(Z.hi,  6));
    }
    {
        iv Z = iv_max((iv){-3,4}, (iv){-5,-1});
        expect(equiv(Z.lo, -3));
        expect(equiv(Z.hi,  4));
    }
}

static void test_sqrt(void) {
    iv Z = iv_sqrt((iv){4,16});
    expect(equiv(Z.lo, 2));
    expect(equiv(Z.hi, 4));
}

static void test_square(void) {
    {
        iv Z = iv_square((iv){3,4});
        expect(equiv(Z.lo,  9));
        expect(equiv(Z.hi, 16));
    }
    {
        iv Z = iv_square((iv){-3,4});
        expect(equiv(Z.lo,  0));
        expect(equiv(Z.hi, 16));
    }
    {
        iv Z = iv_square((iv){-5,-1});
        expect(equiv(Z.lo,  1));
        expect(equiv(Z.hi, 25));
    }
    {
        iv Z = iv_square((iv){0,4});
        expect(equiv(Z.lo,  0));
        expect(equiv(Z.hi, 16));
    }
    {
        iv Z = iv_square((iv){-2,0});
        expect(equiv(Z.lo, 0));
        expect(equiv(Z.hi, 4));
    }
    {
        iv Z = iv_square((iv){0,0});
        expect(equiv(Z.lo, 0));
        expect(equiv(Z.hi, 0));
    }
}

int main(void) {
    test_add();
    test_sub();
    test_mul();
    test_mul4();
    test_min();
    test_max();
    test_sqrt();
    test_square();
    return 0;
}
