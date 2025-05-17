#include "iv.h"
#include <stdio.h>

#define expect(x) if (!(x)) dprintf(2, "%s:%d %s expect(%s)\n", __FILE__, __LINE__, __func__, #x), \
                            __builtin_debugtrap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

static void test_add(void) {
    iv z = iv_add((iv){3,4}, (iv){5,6});
    expect(equiv(z.lo,  8));
    expect(equiv(z.hi, 10));
}

static void test_sub(void) {
    iv z = iv_sub((iv){3,4}, (iv){5,6});
    expect(equiv(z.lo, -3));
    expect(equiv(z.hi, -1));
}

static void test_mul(void) {
    {
        iv z = iv_mul((iv){3,4}, (iv){5,6});
        expect(equiv(z.lo, 15));
        expect(equiv(z.hi, 24));
    }
    {
        iv z = iv_mul((iv){-3,4}, (iv){5,6});
        expect(equiv(z.lo, -18));
        expect(equiv(z.hi,  24));
    }
    {
        iv z = iv_mul((iv){-3,4}, (iv){-5,6});
        expect(equiv(z.lo, -20));
        expect(equiv(z.hi,  24));
    }
    {
        iv z = iv_mul((iv){-3,4}, (iv){-5,-1});
        expect(equiv(z.lo, -20));
        expect(equiv(z.hi,  15));
    }
}

static void test_min(void) {
    {
        iv z = iv_min((iv){3,4}, (iv){5,6});
        expect(equiv(z.lo, 3));
        expect(equiv(z.hi, 4));
    }
    {
        iv z = iv_min((iv){-3,4}, (iv){-5,6});
        expect(equiv(z.lo, -5));
        expect(equiv(z.hi,  4));
    }
    {
        iv z = iv_min((iv){-3,4}, (iv){-5,-1});
        expect(equiv(z.lo, -5));
        expect(equiv(z.hi, -1));
    }
}

static void test_max(void) {
    {
        iv z = iv_max((iv){3,4}, (iv){5,6});
        expect(equiv(z.lo, 5));
        expect(equiv(z.hi, 6));
    }
    {
        iv z = iv_max((iv){-3,4}, (iv){-5,6});
        expect(equiv(z.lo, -3));
        expect(equiv(z.hi,  6));
    }
    {
        iv z = iv_max((iv){-3,4}, (iv){-5,-1});
        expect(equiv(z.lo, -3));
        expect(equiv(z.hi,  4));
    }
}

int main(void) {
    test_add();
    test_sub();
    test_mul();
    test_min();
    test_max();
    return 0;
}
