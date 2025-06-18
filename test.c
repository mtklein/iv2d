#include "cleanup.h"
#include "iv2d.h"
#include "iv2d_vm.h"
#include "stb/stb_image_write.h"
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

static struct iv2d_region const* make_vm_union(float const *cx, float const *cy, float const *cr,
                                               float const *ox, float const *oy) {
    struct iv2d_builder *b = iv2d_builder();
    int center_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,cx)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,cy)));
        int len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        center_circle = iv2d_sub(b, len, iv2d_uni(b,cr));
    }
    int orbit_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,ox)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,oy)));
        int len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        orbit_circle = iv2d_sub(b, len, iv2d_imm(b,100));
    }
    return iv2d_ret(b, iv2d_min(b, center_circle, orbit_circle));
}

int main(int argc, char* argv[]) {
    int w = 800,
        h = 600;
    int quality = 0;
    for (int i = 1; i < argc; i++) {
        int W,H;
        if (sscanf(argv[i], "%dx%d", &W,&H) == 2) { w=W; h=H; continue; }
        if (strspn(argv[i], "+") == strlen(argv[i])) { quality = (int)strlen(argv[i]); }
    }

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h,
                cr = 0.5f * fminf(cx,cy),
                ox = 300, oy = 200;
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region = make_vm_union(&cx,&cy,&cr,&ox,&oy);

    struct image img = {w,h, malloc((size_t)(4*w*h) * sizeof *img.rgba)};
    for (int i = 0; i < 4*w*h; i++) {
        img.rgba[i] = 1.0f;
    }
    iv2d_cover(region, 0,0,w,h, quality, blend_rect,&img);

    unsigned char *px = malloc((size_t)(4*w*h) * sizeof *px);
    for (int i = 0; i < 4*w*h; i++) {
        px[i] = (unsigned char)lrintf(255 * img.rgba[i]);
    }
    stbi_write_png_to_func(write_to_stdout,NULL, w,h,4, px, w*4);
    free(px);
    free(img.rgba);
    return 0;
}

