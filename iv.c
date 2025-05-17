#include "iv.h"
#include <math.h>

static iv iv_containing(float a, float b, float c, float d) {
    return (iv) {
        fminf(fminf(a,b), fminf(c,d)),
        fmaxf(fmaxf(a,b), fmaxf(c,d)),
    };
}

iv iv_add(iv x, iv y) {
    return (iv){
        x.lo + y.lo,
        x.hi + y.hi,
    };
}

iv iv_sub(iv x, iv y) {
    return (iv){
        x.lo - y.hi,
        x.hi - y.lo,
    };
}

iv iv_mul(iv x, iv y) {
    return iv_containing(x.lo * y.lo, x.hi * y.lo,
                         x.lo * y.hi, x.hi * y.hi);
}
