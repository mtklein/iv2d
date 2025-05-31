#pragma once

#include <math.h>

typedef struct {
    float lo,hi;
} iv;

typedef float __attribute__((vector_size(16))) float4;
typedef   int __attribute__((vector_size(16)))   int4;

static inline float4 when(int4 mask, float4 v) {
    return (float4)( mask & (int4)v );
}

typedef struct {
    float4 lo,hi;
} iv4;


static inline iv iv_add(iv X, iv Y) {
    return (iv){
        X.lo + Y.lo,
        X.hi + Y.hi,
    };
}
static inline iv4 iv4_add(iv4 X, iv4 Y) {
    return (iv4){
        X.lo + Y.lo,
        X.hi + Y.hi,
    };
}

static inline iv iv_sub(iv X, iv Y) {
    return (iv){
        X.lo - Y.hi,
        X.hi - Y.lo,
    };
}
static inline iv4 iv4_sub(iv4 X, iv4 Y) {
    return (iv4){
        X.lo - Y.hi,
        X.hi - Y.lo,
    };
}

static inline iv iv_mul(iv X, iv Y) {
    float const a = X.lo * Y.lo,
                b = X.hi * Y.lo,
                c = X.lo * Y.hi,
                d = X.hi * Y.hi;
    return (iv) {
        fminf(fminf(a,d), fminf(b,c)),
        fmaxf(fmaxf(a,d), fmaxf(b,c)),
    };
}
static inline iv4 iv4_mul(iv4 X, iv4 Y) {
    float4 const a = X.lo * Y.lo,
                 b = X.hi * Y.lo,
                 c = X.lo * Y.hi,
                 d = X.hi * Y.hi;
    return (iv4) {
        __builtin_elementwise_min(__builtin_elementwise_min(a,d), __builtin_elementwise_min(b,c)),
        __builtin_elementwise_max(__builtin_elementwise_max(a,d), __builtin_elementwise_max(b,c)),
    };
}

static inline iv iv_min(iv X, iv Y) {
    return (iv){
        fminf(X.lo, Y.lo),
        fminf(X.hi, Y.hi),
    };
}
static inline iv4 iv4_min(iv4 X, iv4 Y) {
    return (iv4){
        __builtin_elementwise_min(X.lo, Y.lo),
        __builtin_elementwise_min(X.hi, Y.hi),
    };
}

static inline iv iv_max(iv X, iv Y) {
    return (iv){
        fmaxf(X.lo, Y.lo),
        fmaxf(X.hi, Y.hi),
    };
}
static inline iv4 iv4_max(iv4 X, iv4 Y) {
    return (iv4){
        __builtin_elementwise_max(X.lo, Y.lo),
        __builtin_elementwise_max(X.hi, Y.hi),
    };
}

static inline iv iv_neg(iv X) {
    return iv_sub((iv){0}, X);
}
static inline iv4 iv4_neg(iv4 X) {
    return iv4_sub((iv4){0}, X);
}

static inline iv iv_sqrt(iv X) {
    return (iv){
        sqrtf(X.lo),
        sqrtf(X.hi),
    };
}
static inline iv4 iv4_sqrt(iv4 X) {
    return (iv4){
        __builtin_elementwise_sqrt(X.lo),
        __builtin_elementwise_sqrt(X.hi),
    };
}

static inline iv iv_square(iv X) {
    float const a2 = X.lo * X.lo,
                b2 = X.hi * X.hi;
    if (X.lo > 0 || X.hi < 0) {
        return (iv){fminf(a2,b2), fmaxf(a2,b2)};
    } else {
        return (iv){           0, fmaxf(a2,b2)};
    }
}
static inline iv4 iv4_square(iv4 X) {
    float4 const a2 = X.lo * X.lo,
                 b2 = X.hi * X.hi;
    return (iv4){
        when(X.lo > 0 | X.hi < 0, __builtin_elementwise_min(a2,b2)),
                                  __builtin_elementwise_max(a2,b2) ,
    };
}
