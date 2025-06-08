#pragma once

#include <stdio.h>

#define expect(x) if (!(x)) dprintf(2, "%s:%d %s expect(%s)\n", __FILE__, __LINE__, __func__, #x), \
                            __builtin_debugtrap()

static inline _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}
