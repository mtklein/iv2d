#include "../iv2d_vm.h"
#include "slides.h"
#include <stdlib.h>

static struct iv2d_region const* create(float const *w, float const *h, float const *t) {
    struct iv2d_builder *b = iv2d_builder();
    int center_circle;
    {
        int const cx = iv2d_mul(b, iv2d_imm(b, 0.5f), iv2d_uni(b,w)),
                  cy = iv2d_mul(b, iv2d_imm(b, 0.5f), iv2d_uni(b,h)),
                  cr = iv2d_mul(b, iv2d_imm(b, 0.5f), iv2d_min(b, cx,cy));

        int const dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), cx)),
                  dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), cy)),
                  len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        center_circle = iv2d_sub(b, len, cr);
    }
    int orbit_circle;
    {
        int const ox = iv2d_add(b, cx,
                                iv2d_mul(b, iv2d_imm(b, 100),
                                         iv2d_cos(b, iv2d_uni(b,t)))),
                  oy = iv2d_add(b, cy,
                                iv2d_mul(b, iv2d_imm(b, 100),
                                         iv2d_sin(b, iv2d_uni(b,t))));
        int const dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), ox)),
                  dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), oy)),
                  len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
        orbit_circle = iv2d_sub(b, len, iv2d_imm(b,100));
    }
    return iv2d_ret(b, iv2d_min(b, center_circle, orbit_circle));
}

static void cleanup(struct iv2d_region const *p) {
    free((void*)(uintptr_t)p);
}

extern struct slide vm_union;
struct slide vm_union = { "vm_union", create, cleanup };
