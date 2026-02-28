#include "slides.h"
#include "iv2d_regions.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang attribute push (__attribute__((no_sanitize("integer", "undefined"))), apply_to=function)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#pragma clang attribute pop
#pragma clang diagnostic pop
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ctx {
    uint8_t *pixels;
    int w;
    int :32;
};

static void paint_rect(void *arg, float l, float t, float r, float b, float cov) {
    struct ctx *ctx = arg;
    int const x0 = (int)l, y0 = (int)t,
              x1 = (int)r, y1 = (int)b;
    unsigned const a = (unsigned)(uint8_t)(cov * 255);
    if (!a) { return; }

    // Blend (127,127,127,a) over white (255,255,255,255).
    // Match SDL2 geometry renderer's >>8 approximation for division by 255.
    uint8_t const gray = (uint8_t)((127u * a + 255u * (256u - a)) >> 8);
    uint8_t const   oa = (uint8_t)(a + ((255u * (255u - a)) >> 8));

    for (int y = y0; y < y1; y++) {
        uint8_t *row = ctx->pixels + y * ctx->w * 4 + x0 * 4;
        for (int x = x0; x < x1; x++) {
            row[0] = gray;
            row[1] = gray;
            row[2] = gray;
            row[3] = oa;
            row += 4;
        }
    }
}

static void write_to_stdout(void *ctx, void *buf, int len) {
    (void)ctx;
    fwrite(buf, 1, (size_t)len, stdout);
}

// Usage: gold <slide> <quality> [WxH] [-s]
int main(int argc, char *argv[]) {
    int slide_idx = 0, quality = 0, w = 800, h = 600;
    _Bool stroke = false, have_slide = false;

    for (int i = 1; i < argc; i++) {
        int W,H;
        if (2 == sscanf(argv[i], "%dx%d", &W, &H)) { w = W; h = H; continue; }
        if (0 == strcmp(argv[i], "-s")) { stroke = true; continue; }

        int n;
        if (1 == sscanf(argv[i], "%d", &n)) {
            if (!have_slide) { slide_idx = n; have_slide = true; }
            else             { quality = n; }
            continue;
        }
    }

    struct slides slides;
    slides_init(&slides, w, h, 0);

    if (slide_idx < 0 || slide_idx >= slides.count) {
        fprintf(stderr, "slide %d out of range [0,%d)\n", slide_idx, slides.count);
        slides_fini(&slides);
        return 1;
    }

    struct iv2d_region const *region = slides.slide[slide_idx].region;
    struct iv2d_stroke stroke_r = {.region={iv2d_stroke}, region, 2};
    if (stroke) {
        region = &stroke_r.region;
    }

    uint8_t *pixels = malloc((size_t)(w * h) * 4);
    memset(pixels, 255, (size_t)(w * h) * 4);

    struct ctx ctx = { pixels, w };
    iv2d_cover(region, 0,0,w,h, quality, paint_rect,&ctx);

    slides_fini(&slides);

    stbi_write_png_to_func(write_to_stdout, NULL, w,h,4, pixels, w*4);

    free(pixels);
    return 0;
}
