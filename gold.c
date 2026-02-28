#include "slides.h"
#include "iv2d_regions.h"
#include "len.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang attribute push (__attribute__((no_sanitize("integer", "undefined"))), apply_to=function)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#pragma clang attribute pop
#pragma clang diagnostic pop
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct quad {
    struct { float x,y; SDL_Color c; } vertex[6];
};

struct ctx {
    struct quad *quad;
    int quads, quad_cap;
};

static void queue_rect(void *arg, float l, float t, float r, float b, float cov) {
    struct ctx *ctx = arg;
    if (ctx->quad_cap == ctx->quads) {
        ctx->quad_cap = ctx->quad_cap ? 2 * ctx->quad_cap : 1;
        ctx->quad = realloc(ctx->quad, (size_t)ctx->quad_cap * sizeof *ctx->quad);
    }
    SDL_Color const c = {127,127,127, (uint8_t)(cov*255)};
    ctx->quad[ctx->quads++] = (struct quad) {{
        {l,t,c}, {r,t,c}, {l,b,c},
        {r,t,c}, {r,b,c}, {l,b,c},
    }};
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

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
        0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor    (renderer, 255,255,255,255);
    SDL_RenderClear           (renderer);

    struct slides slides;
    slides_init(&slides, w, h, 0);

    if (slide_idx < 0 || slide_idx >= slides.count) {
        fprintf(stderr, "slide %d out of range [0,%d)\n", slide_idx, slides.count);
        slides_fini(&slides);
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(surface);
        return 1;
    }

    struct iv2d_region const *region = slides.slide[slide_idx].region;
    struct iv2d_stroke stroke_r = {.region={iv2d_stroke}, region, 2};
    if (stroke) {
        region = &stroke_r.region;
    }

    struct ctx ctx = {0};
    iv2d_cover(region, 0,0,w,h, quality, queue_rect,&ctx);

    slides_fini(&slides);

    SDL_RenderGeometryRaw(renderer, NULL
                                  , &ctx.quad->vertex->x, sizeof *ctx.quad->vertex
                                  , &ctx.quad->vertex->c, sizeof *ctx.quad->vertex
                                  , NULL, 0
                                  , 6 * ctx.quads
                                  , NULL, 0, 0);

    SDL_Surface *rgba = SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
    SDL_RenderReadPixels(renderer, NULL,
                         SDL_PIXELFORMAT_RGBA32, rgba->pixels, rgba->pitch);
    stbi_write_png_to_func(write_to_stdout, NULL, w,h,4, rgba->pixels, rgba->pitch);

    SDL_FreeSurface(rgba);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    free(ctx.quad);
    return 0;
}
