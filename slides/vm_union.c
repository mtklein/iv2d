#include "../iv2d_vm.h"
#include "slides.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

static struct iv2d_region const* create(float w, float h, float t) {
    struct iv2d_builder *b = iv2d_builder();
    float cx = 0.5f * w;
    float cy = 0.5f * h;
    float cr = 0.5f * fminf(cx, cy);
    float ox = cx + (300 - cx)*cosf(t) - (200 - cy)*sinf(t);
    float oy = cy + (200 - cy)*cosf(t) + (300 - cx)*sinf(t);

    int center_circle;
    {
        int const dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_imm(b, cx))),
                  dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_imm(b, cy))),
                  len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        center_circle = iv2d_sub(b, len, iv2d_imm(b, cr));
    }
    int orbit_circle;
    {
        int dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_imm(b, ox)));
        int dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_imm(b, oy)));
        int len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        orbit_circle = iv2d_sub(b, len, iv2d_imm(b, 100));
    }
    return iv2d_ret(b, iv2d_min(b, center_circle, orbit_circle));
}

static void cleanup(struct iv2d_region const *p) {
    free((void*)(uintptr_t)p);
}

extern struct slide vm_union;
struct slide vm_union = { "vm_union", create, cleanup };
