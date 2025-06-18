#include "cleanup.h"
#include "iv2d.h"
#include "iv2d_vm.h"
#include "stb_image_write.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static void write_to_stdout(void *ctx, const void *buf, int len) {
    (void)ctx;
    fwrite(buf, 1, (size_t)len, stdout);
}

struct image {
    int w,h;
    float *px; // RGBA as floats
};

static void blend_rect(void *ctx, float l, float t, float r, float b, float cov) {
    struct image *img = ctx;
    int x0 = (int)l;
    int y0 = (int)t;
    int x1 = (int)r;
    int y1 = (int)b;
    float a = cov;
    float c = 0.5f * a;
    for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
        float *dst = img->px + 4*(y*img->w + x);
        dst[0] = c + dst[0]*(1-a);
        dst[1] = c + dst[1]*(1-a);
        dst[2] = c + dst[2]*(1-a);
        dst[3] = a + dst[3]*(1-a);
    }
}

static struct iv2d_region const* make_vm_union(float cx,float cy,float cr,float ox,float oy) {
    struct iv2d_builder *b = iv2d_builder();
    int center_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&cx)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&cy)));
        int len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        center_circle = iv2d_sub(b, len, iv2d_uni(b,&cr));
    }
    int orbit_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&ox)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&oy)));
        int len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        orbit_circle = iv2d_sub(b, len, iv2d_imm(b,100));
    }
    return iv2d_ret(b, iv2d_min(b, center_circle, orbit_circle));
}

int main(int argc, char **argv) {
    int w=800, h=600;
    int quality=0;
    for (int i=1; i<argc; i++) {
        int W,H;
        if (sscanf(argv[i], "%dx%d", &W,&H) == 2) { w=W; h=H; continue; }
        if (strspn(argv[i], "+") == strlen(argv[i])) { quality = (int)strlen(argv[i]); }
    }

    float cx = 0.5f * (float)w;
    float cy = 0.5f * (float)h;
    float cr = 0.5f * fminf(cx, cy);
    float ox = 300;
    float oy = 200;

    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region = make_vm_union(cx,cy,cr,ox,oy);

    struct image img = {w,h,calloc((size_t)w * (size_t)h * 4, sizeof *img.px)};
    iv2d_cover(region, 0,0,w,h, quality, blend_rect, &img);

    unsigned char *pixels = malloc((size_t)w * (size_t)h * 4);
    for (int i=0; i<w*h; i++) {
        for (int c=0; c<4; c++) {
            float v = img.px[4*i+c];
            pixels[4*i+c] = (unsigned char)lrintf(255 * v);
        }
    }
    stbi_write_png_to_func(write_to_stdout,NULL, w,h,4, pixels, w*4);
    free(pixels);
    free(img.px);
    return 0;
}

