#include "iv2d.h"
#include <stdint.h>

static iv2d_color ld_rgba_fp16(void const *px) {
    _Float16 const *rgba = px;
    return (iv2d_color){
        rgba[0],
        rgba[1],
        rgba[2],
        rgba[3],
    };
}
static void st_rgba_fp16(void *px, iv2d_color c) {
    _Float16 *rgba = px;
    *rgba++ = c.r;
    *rgba++ = c.g;
    *rgba++ = c.b;
    *rgba++ = c.a;
}
iv2d_format const iv2d_format_rgba_fp16 = {
    .ld  = ld_rgba_fp16,
    .st  = st_rgba_fp16,
    .bpp = 8,
};

static iv2d_color ld_rgba_fp32(void const *px) {
    float const *rgba = px;
    return (iv2d_color) {
        (_Float16)rgba[0],
        (_Float16)rgba[1],
        (_Float16)rgba[2],
        (_Float16)rgba[3],
    };
}
static void st_rgba_fp32(void *px, iv2d_color c) {
    float *rgba = px;
    *rgba++ = (float)c.r;
    *rgba++ = (float)c.g;
    *rgba++ = (float)c.b;
    *rgba++ = (float)c.a;
}
iv2d_format const iv2d_format_rgba_fp32 = {
    .ld  = ld_rgba_fp32,
    .st  = st_rgba_fp32,
    .bpp = 16,
};

static iv2d_color iv2d_shade_color_fn(float x, float y, void *ctx) {
    (void)x;
    (void)y;
    return *(iv2d_color const*)ctx;
}

iv2d_shade iv2d_shade_color(iv2d_color const *c) {
    return (iv2d_shade) {
        .fn  = iv2d_shade_color_fn,
        .ctx = (void*)(uintptr_t)c,
    };
}

iv2d_color iv2d_blend_src(iv2d_color src, iv2d_color dst) {
    (void)dst;
    return src;
}
iv2d_color iv2d_blend_srcover(iv2d_color src, iv2d_color dst) {
    src.r += dst.r * (1 - src.a);
    src.g += dst.g * (1 - src.a);
    src.b += dst.b * (1 - src.a);
    src.a += dst.a * (1 - src.a);
    return src;
}
