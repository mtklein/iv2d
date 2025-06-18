#pragma once

int dprintf(int, char const*, ...);
#define expect(x) if (!(x)) dprintf(2, "%s:%d %s expect(%s)\n", __FILE__, __LINE__, __func__, #x), \
                            __builtin_debugtrap()

static inline __attribute__((overloadable)) _Bool equiv(float x, float y) {
    return (x <= y && y <= x) || (x != x && y != y);
}

static inline __attribute__((overloadable)) _Bool equiv(_Float16 x, _Float16 y) {
    return equiv((float)x, (float)y);
}
