#include "iv.h"
#include <math.h>

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
    float const a = x.lo * y.lo,
                b = x.hi * y.lo,
                c = x.lo * y.hi,
                d = x.hi * y.hi;
    return (iv) {
        fminf(fminf(a,b), fminf(c,d)),
        fmaxf(fmaxf(a,b), fmaxf(c,d)),
    };
}

iv iv_min(iv x, iv y) {
    return (iv){
        fminf(x.lo, y.lo),
        fminf(x.hi, y.hi),
    };
}

iv iv_max(iv x, iv y) {
    return (iv){
        fmaxf(x.lo, y.lo),
        fmaxf(x.hi, y.hi),
    };
}

iv iv_sqrt(iv x) {
    return (iv){
        sqrtf(x.lo),
        sqrtf(x.hi),
    };
}
