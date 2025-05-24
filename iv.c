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
        fminf(fminf(a,d), fminf(b,c)),
        fmaxf(fmaxf(a,d), fmaxf(b,c)),
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

iv iv_square(iv x) {
    // [+a,+b]^2 = [a^2, b^2]
    // [-a,-b]^2 = [b^2, a^2]
    // [-a,+b]^2 = [-a,+b] x [-a,+b] = [-ab, max(a^2,b^2)]
    float const a2 = x.lo * x.lo,
                ab = x.lo * x.hi,
                b2 = x.hi * x.hi;
    return (iv){
        fminf(ab, fminf(a2, b2)),
                  fmaxf(a2, b2) ,
    };
}
