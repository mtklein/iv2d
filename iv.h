#pragma once

typedef float __attribute__((vector_size(16))) float4;
typedef   int __attribute__((vector_size(16)))   int4;

typedef _Float16 __attribute__((vector_size(8))) float4h;
typedef short    __attribute__((vector_size(8)))  int4h;

static inline float4 if_then_else(int4 mask, float4 t, float4 e) {
    return (float4)( ( mask & (int4)t)
                   | (~mask & (int4)e) );
}

static inline float4 when(int4 mask, float4 v) {
    return if_then_else(mask, v, (float4){0});
}

typedef struct {
    float4 lo,hi;
} iv;

static inline iv as_iv(float x) {
    return (iv){{x,x,x,x}, {x,x,x,x}};
}

static inline iv iv_add(iv x, iv y) {
    return (iv){
        x.lo + y.lo,
        x.hi + y.hi,
    };
}

static inline iv iv_sub(iv x, iv y) {
    return (iv){
        x.lo - y.hi,
        x.hi - y.lo,
    };
}

static inline iv iv_mul(iv x, iv y) {
    float4 const a = x.lo * y.lo,
                 b = x.hi * y.lo,
                 c = x.lo * y.hi,
                 d = x.hi * y.hi;
    return (iv){
        __builtin_elementwise_min(__builtin_elementwise_min(a,b), __builtin_elementwise_min(c,d)),
        __builtin_elementwise_max(__builtin_elementwise_max(a,b), __builtin_elementwise_max(c,d)),
    };
}

static inline iv iv_mad(iv x, iv y, iv z) {
    return iv_add(iv_mul(x,y), z);
}

static inline iv iv_min(iv x, iv y) {
    return (iv){
        __builtin_elementwise_min(x.lo, y.lo),
        __builtin_elementwise_min(x.hi, y.hi),
    };
}

static inline iv iv_max(iv x, iv y) {
    return (iv){
        __builtin_elementwise_max(x.lo, y.lo),
        __builtin_elementwise_max(x.hi, y.hi),
    };
}

static inline iv iv_sqrt(iv x) {
    return (iv){
        __builtin_elementwise_sqrt(x.lo),
        __builtin_elementwise_sqrt(x.hi),
    };
}

static inline iv iv_square(iv x) {
    float4 const a2 = x.lo * x.lo,
                 b2 = x.hi * x.hi;
    return (iv){
        when(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a2,b2)),
                                  __builtin_elementwise_max(a2,b2) ,
    };
}

static inline iv iv_abs(iv x) {
    float4 const a = __builtin_elementwise_abs(x.lo),
                 b = __builtin_elementwise_abs(x.hi);
    return (iv){
        when(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a,b)),
                                  __builtin_elementwise_max(a,b) ,
    };
}

static inline iv iv_inv(iv x) {
    return (iv) {
        if_then_else(x.lo > 0 | x.hi < 0 | (x.lo >= 0 & x.hi > 0), 1/x.hi, (float4){0} - 1/0.0f),
        if_then_else(x.lo > 0 | x.hi < 0 | (x.lo < 0 & x.hi <= 0), 1/x.lo, (float4){0} + 1/0.0f),
    };
}

static inline float4h if_then_else16(int4h mask, float4h t, float4h e) {
    return (float4h)( (mask & (int4h)t)
                    | (~mask & (int4h)e) );
}

static inline float4h when16(int4h mask, float4h v) {
    return if_then_else16(mask, v, (float4h){0});
}

typedef struct {
    float4h lo, hi;
} iv16;

static inline iv16 as_iv16(_Float16 x) {
    return (iv16){
        (float4h){x,x,x,x},
        (float4h){x,x,x,x},
    };
}

static inline iv16 iv16_add(iv16 x, iv16 y) {
    return (iv16){
        x.lo + y.lo,
        x.hi + y.hi,
    };
}

static inline iv16 iv16_sub(iv16 x, iv16 y) {
    return (iv16){
        x.lo - y.hi,
        x.hi - y.lo,
    };
}

static inline iv16 iv16_mul(iv16 x, iv16 y) {
    float4h const a = x.lo * y.lo,
                   b = x.hi * y.lo,
                   c = x.lo * y.hi,
                   d = x.hi * y.hi;
    return (iv16){
        __builtin_elementwise_min(__builtin_elementwise_min(a,b),
                                  __builtin_elementwise_min(c,d)),
        __builtin_elementwise_max(__builtin_elementwise_max(a,b),
                                  __builtin_elementwise_max(c,d)),
    };
}

static inline iv16 iv16_mad(iv16 x, iv16 y, iv16 z) {
    return iv16_add(iv16_mul(x,y), z);
}

static inline iv16 iv16_min(iv16 x, iv16 y) {
    return (iv16){
        __builtin_elementwise_min(x.lo, y.lo),
        __builtin_elementwise_min(x.hi, y.hi),
    };
}

static inline iv16 iv16_max(iv16 x, iv16 y) {
    return (iv16){
        __builtin_elementwise_max(x.lo, y.lo),
        __builtin_elementwise_max(x.hi, y.hi),
    };
}

static inline iv16 iv16_sqrt(iv16 x) {
    return (iv16){
        __builtin_elementwise_sqrt(x.lo),
        __builtin_elementwise_sqrt(x.hi),
    };
}

static inline iv16 iv16_square(iv16 x) {
    float4h const a2 = x.lo * x.lo,
                   b2 = x.hi * x.hi;
    return (iv16){
        when16(x.lo > 0 | x.hi < 0,
               __builtin_elementwise_min(a2,b2)),
                                      __builtin_elementwise_max(a2,b2),
    };
}

static inline iv16 iv16_abs(iv16 x) {
    float4h const a = __builtin_elementwise_abs(x.lo),
                   b = __builtin_elementwise_abs(x.hi);
    return (iv16){
        when16(x.lo > 0 | x.hi < 0,
               __builtin_elementwise_min(a,b)),
                                      __builtin_elementwise_max(a,b),
    };
}

static inline iv16 iv16_inv(iv16 x) {
    return (iv16){
        if_then_else16(x.lo > 0 | x.hi < 0 | (x.lo >= 0 & x.hi > 0),
                       1/x.hi, (float4h){0} - 1/0.0f),
        if_then_else16(x.lo > 0 | x.hi < 0 | (x.lo < 0 & x.hi <= 0),
                       1/x.lo, (float4h){0} + 1/0.0f),
    };
}
