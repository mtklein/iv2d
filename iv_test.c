#include "iv.h"
#include <stdio.h>

#define expect(x) if (!(x)) dprintf(2, "%s:%d %s expect(%s)\n", __FILE__, __LINE__, __func__, #x), \
                            __builtin_debugtrap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

static void test_basics(void) {
    iv z = iv_add((iv){3,4}, (iv){5,6});
    expect(equiv(z.lo,  8));
    expect(equiv(z.hi, 10));
}

int main(void) {
    test_basics();
    return 0;
}
