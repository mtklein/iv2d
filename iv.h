#pragma once

#include <math.h>

typedef struct {
    float lo,hi;
} iv;


inline iv iv_add(iv X, iv Y) {
    return (iv){
        X.lo + Y.lo,
        X.hi + Y.hi,
    };
}

inline iv iv_sub(iv X, iv Y) {
    return (iv){
        X.lo - Y.hi,
        X.hi - Y.lo,
    };
}

inline iv iv_mul(iv X, iv Y) {
    float const a = X.lo * Y.lo,
                b = X.hi * Y.lo,
                c = X.lo * Y.hi,
                d = X.hi * Y.hi;
    return (iv) {
        fminf(fminf(a,d), fminf(b,c)),
        fmaxf(fmaxf(a,d), fmaxf(b,c)),
    };
}

inline iv iv_min(iv X, iv Y) {
    return (iv){
        fminf(X.lo, Y.lo),
        fminf(X.hi, Y.hi),
    };
}

inline iv iv_max(iv X, iv Y) {
    return (iv){
        fmaxf(X.lo, Y.lo),
        fmaxf(X.hi, Y.hi),
    };
}

inline iv iv_neg(iv X) {
    return iv_sub((iv){0,0}, X);
}

inline iv iv_sqrt(iv X) {
    return (iv){
        sqrtf(X.lo),
        sqrtf(X.hi),
    };
}

inline iv iv_square(iv X) {
    // [+a,+b]^2 = [a^2, b^2]
    // [-a,-b]^2 = [b^2, a^2]
    // [-a,+b]^2 = [-a,+b] x [-a,+b] = [-ab, max(a^2,b^2)]
    float const a2 = X.lo * X.lo,
                ab = X.lo * X.hi,
                b2 = X.hi * X.hi;
    return (iv){
        fminf(ab, fminf(a2, b2)),
                  fmaxf(a2, b2) ,
    };
}
