#pragma clang diagnostic ignored "-Wcast-qual"
#include "../slide.h"
#include "../iv2d_vm.h"
#include "../iv2d_regions.h"
#include <math.h>

static struct iv2d_region const *make_vm_union(struct slide *s, float w, float h, double t) {
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float cr = 0.5f * fminf(cx, cy);
    float th = (float)t;
    float ox = cx + (300 - cx) * cosf(th) - (200 - cy) * sinf(th);
    float oy = cy + (200 - cy) * cosf(th) + (300 - cx) * sinf(th);

    if (s->region) {
        free((void*)s->region);
        s->region = NULL;
    }

    struct iv2d_builder *b = iv2d_builder();
    int center_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b, &cx)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b, &cy)));
        int len  = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        center_circle = iv2d_sub(b, len, iv2d_uni(b, &cr));
    }
    int orbit_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b, &ox)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b, &oy)));
        int len  = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        orbit_circle = iv2d_sub(b, len, iv2d_imm(b, 100));
    }
    return iv2d_ret(b, iv2d_min(b, center_circle, orbit_circle));
}

struct slide vm_union_slide = {"vm union", NULL, make_vm_union, 0};
