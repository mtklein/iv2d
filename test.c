#include "iv2d.h"
#include "iv2d_regions.h"
#include "iv2d_vm.h"
#include "stb/stb_image_write.h"
#include "slides/slides.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_to_stdout(void *ctx, void *buf, int len) {
    (void)ctx;
    fwrite(buf, 1, (size_t)len, stdout);
}

struct image {
    int    w,h;
    float *rgba;
};

static void blend_rect(void *ctx, float l, float t, float r, float b, float a) {
    struct image *img = ctx;
    for (int y = (int)t; y < (int)b; y++)
    for (int x = (int)l; x < (int)r; x++) {
        float *dst = img->rgba + 4*(y*img->w + x);
        float const grey = 0.5f;
        dst[0] = grey*a + dst[0]*(1-a);
        dst[1] = grey*a + dst[1]*(1-a);
        dst[2] = grey*a + dst[2]*(1-a);
        dst[3] =      a + dst[3]*(1-a);
    }
}

int main(int argc, char* argv[]) {
    int w = 800,
        h = 600;
    int quality = 0;
    int n = 0;
    int stroke = 0;
    for (int i = 1; i < argc; i++) {
        int W,H;
        if (sscanf(argv[i], "%dx%d", &W,&H) == 2) { w=W; h=H; continue; }
        if (strspn(argv[i], "+") == strlen(argv[i])) { quality = (int)strlen(argv[i]); continue; }
        if (strcmp(argv[i], "s") == 0) { stroke = 1; continue; }
        if (sscanf(argv[i], "%d", &n) == 1) { continue; }
    }

    struct image img = {w,h, malloc((size_t)(4*w*h) * sizeof *img.rgba)};
    {
        float const W=(float)w, H=(float)h, t=0.0f;
        struct iv2d_region const *region = slide[n]->create(W,H,t);
        struct iv2d_stroke stroke_region = {.region={iv2d_stroke}, region, 2};
        stroke_region.arg = region;
        if (stroke) {
            region = &stroke_region.region;
        }

        for (int i = 0; i < 4*w*h; i++) {
            img.rgba[i] = 1.0f;
        }
        iv2d_cover(region, 0,0,w,h, quality, blend_rect,&img);
        slide[n]->cleanup(stroke ? stroke_region.arg : region);
    }
    {
        unsigned char *px = malloc((size_t)(4*w*h) * sizeof *px);
        for (int i = 0; i < 4*w*h; i++) {
            px[i] = (unsigned char)lrintf(255 * img.rgba[i]);
        }
        stbi_write_png_to_func(write_to_stdout,NULL, w,h,4, px, w*4);
        free(px);
    }
    free(img.rgba);
    return 0;
}

