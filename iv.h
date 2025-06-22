#pragma once

typedef float __attribute__((vector_size(16))) float4;
typedef   int __attribute__((vector_size(16)))   int4;

typedef _Float16 __attribute__((vector_size(8))) half4;
typedef short    __attribute__((vector_size(8))) short4;

static inline float4 if_then_else(int4 mask, float4 t, float4 e) {
    return (float4)( ( mask & (int4)t)
                   | (~mask & (int4)e) );
}

static inline float4 when(int4 mask, float4 v) {
    return if_then_else(mask, v, (float4){0});
}

typedef struct {
    float4 lo,hi;
} iv32;

static inline iv32 as_iv32(float x) {
    return (iv32){{x,x,x,x}, {x,x,x,x}};
}

static inline iv32 iv32_add(iv32 x, iv32 y) {
    return (iv32){
        x.lo + y.lo,
        x.hi + y.hi,
    };
}

static inline iv32 iv32_sub(iv32 x, iv32 y) {
    return (iv32){
        x.lo - y.hi,
        x.hi - y.lo,
    };
}

static inline iv32 iv32_mul(iv32 x, iv32 y) {
    float4 const a = x.lo * y.lo,
                 b = x.hi * y.lo,
                 c = x.lo * y.hi,
                 d = x.hi * y.hi;
    return (iv32){
        __builtin_elementwise_min(__builtin_elementwise_min(a,b), __builtin_elementwise_min(c,d)),
        __builtin_elementwise_max(__builtin_elementwise_max(a,b), __builtin_elementwise_max(c,d)),
    };
}

static inline iv32 iv32_mad(iv32 x, iv32 y, iv32 z) {
    return iv32_add(iv32_mul(x,y), z);
}

static inline iv32 iv32_min(iv32 x, iv32 y) {
    return (iv32){
        __builtin_elementwise_min(x.lo, y.lo),
        __builtin_elementwise_min(x.hi, y.hi),
    };
}

static inline iv32 iv32_max(iv32 x, iv32 y) {
    return (iv32){
        __builtin_elementwise_max(x.lo, y.lo),
        __builtin_elementwise_max(x.hi, y.hi),
    };
}

static inline iv32 iv32_sqrt(iv32 x) {
    return (iv32){
        __builtin_elementwise_sqrt(x.lo),
        __builtin_elementwise_sqrt(x.hi),
    };
}

static inline iv32 iv32_square(iv32 x) {
    float4 const a2 = x.lo * x.lo,
                 b2 = x.hi * x.hi;
    return (iv32){
        when(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a2,b2)),
                                  __builtin_elementwise_max(a2,b2) ,
    };
}

static inline iv32 iv32_abs(iv32 x) {
    float4 const a = __builtin_elementwise_abs(x.lo),
                 b = __builtin_elementwise_abs(x.hi);
    return (iv32){
        when(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a,b)),
                                  __builtin_elementwise_max(a,b) ,
    };
}

static inline iv32 iv32_inv(iv32 x) {
    return (iv32) {
        if_then_else(x.lo > 0 | x.hi < 0 | (x.lo >= 0 & x.hi > 0), 1/x.hi, (float4){0} - 1/0.0f),
        if_then_else(x.lo > 0 | x.hi < 0 | (x.lo < 0 & x.hi <= 0), 1/x.lo, (float4){0} + 1/0.0f),
    };
}

static inline iv32 iv32_div(iv32 x, iv32 y) {
    return iv32_mul(x, iv32_inv(y));
}

static inline half4 if_then_else16(short4 mask, half4 t, half4 e) {
    return (half4)( ( mask & (short4)t)
                  | (~mask & (short4)e) );
}

static inline half4 when16(short4 mask, half4 v) {
    return if_then_else16(mask, v, (half4){0});
}

typedef struct {
    half4 lo, hi;
} iv16;

static inline iv16 as_iv16(_Float16 x) {
    return (iv16){
        (half4){x,x,x,x},
        (half4){x,x,x,x},
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
    half4 const a = x.lo * y.lo,
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
    half4 const a2 = x.lo * x.lo,
                b2 = x.hi * x.hi;
    return (iv16){
        when16(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a2,b2)),
                                    __builtin_elementwise_max(a2,b2) ,
    };
}

static inline iv16 iv16_abs(iv16 x) {
    half4 const a = __builtin_elementwise_abs(x.lo),
                b = __builtin_elementwise_abs(x.hi);
    return (iv16){
        when16(x.lo > 0 | x.hi < 0, __builtin_elementwise_min(a,b)),
                                    __builtin_elementwise_max(a,b) ,
    };
}

static inline iv16 iv16_inv(iv16 x) {
    return (iv16){
        if_then_else16(x.lo > 0 | x.hi < 0 | (x.lo >= 0 & x.hi > 0), 1/x.hi, (half4){0} - 1/0.0f),
        if_then_else16(x.lo > 0 | x.hi < 0 | (x.lo < 0 & x.hi <= 0), 1/x.lo, (half4){0} + 1/0.0f),
    };
}

static inline iv16 iv16_div(iv16 x, iv16 y) {
    return iv16_mul(x, iv16_inv(y));
}
