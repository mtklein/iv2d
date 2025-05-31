#pragma once

typedef float __attribute__((vector_size(16))) float4;
typedef   int __attribute__((vector_size(16)))   int4;

static inline float4 when(int4 mask, float4 v) {
    return (float4)( mask & (int4)v );
}

typedef struct {
    float4 lo,hi;
} iv;


static inline iv iv_add(iv X, iv Y) {
    return (iv){
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

static inline iv iv_mul(iv X, iv Y) {
    float4 const a = X.lo * Y.lo,
                 b = X.hi * Y.lo,
                 c = X.lo * Y.hi,
                 d = X.hi * Y.hi;
    return (iv) {
        __builtin_elementwise_min(__builtin_elementwise_min(a,d), __builtin_elementwise_min(b,c)),
        __builtin_elementwise_max(__builtin_elementwise_max(a,d), __builtin_elementwise_max(b,c)),
    };
}

static inline iv iv_min(iv X, iv Y) {
    return (iv){
        __builtin_elementwise_min(X.lo, Y.lo),
        __builtin_elementwise_min(X.hi, Y.hi),
    };
}

static inline iv iv_max(iv X, iv Y) {
    return (iv){
        __builtin_elementwise_max(X.lo, Y.lo),
        __builtin_elementwise_max(X.hi, Y.hi),
    };
}

static inline iv iv_neg(iv X) {
    return iv_sub((iv){0}, X);
}

static inline iv iv_sqrt(iv X) {
    return (iv){
        __builtin_elementwise_sqrt(X.lo),
        __builtin_elementwise_sqrt(X.hi),
    };
}

static inline iv iv_square(iv X) {
    float4 const a2 = X.lo * X.lo,
                 b2 = X.hi * X.hi;
    return (iv){
        when(X.lo > 0 | X.hi < 0, __builtin_elementwise_min(a2,b2)),
                                  __builtin_elementwise_max(a2,b2) ,
    };
}
