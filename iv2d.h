#pragma once
#include <stddef.h>

typedef struct {
    _Float16 r,g,b,a;
} iv2d_color;

typedef struct {
    iv2d_color (*ld)(void const *px);
    void (*st)(void *px, iv2d_color);
    size_t bpp;
} iv2d_format;
extern iv2d_format const iv2d_format_rgba_fp16,
                         iv2d_format_rgba_fp32;

typedef struct {
    iv2d_format const *fmt;
    void              *px;
    size_t             rb;
    int                w,h;
} iv2d_surface;

typedef struct {
    iv2d_color (*fn)(float x, float y, void *ctx);
    void        *ctx;
} iv2d_shade;
iv2d_shade iv2d_shade_color(iv2d_color const*);

typedef iv2d_color (iv2d_blend)(iv2d_color src, iv2d_color dst);
iv2d_blend iv2d_blend_src,
           iv2d_blend_srcover;

void iv2d_draw_unit_square(iv2d_surface*,
                           iv2d_shade const*,
                           iv2d_blend*);
