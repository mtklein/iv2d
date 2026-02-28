#pragma once

#include "iv2d.h"
#include "iv2d_regions.h"
#include "iv2d_vm.h"

struct slide {
    struct iv2d_region const *region;
    char const *name;
};

enum { SLIDES_HP = 7 };

struct slides {
    float cx, cy, cr, ox, oy;
    int :32;

    struct iv2d_circle center, orbit;
    struct iv2d_invert invorb;
    struct iv2d_region const *center_orbit [2],
                             *center_invorb[2];
    struct iv2d_setop union_, intersect, difference;

    struct iv2d_capsule capsule;
    struct iv2d_halfplane halfplane;

    struct iv2d_halfplane hp[SLIDES_HP];
    struct iv2d_region const *ngon_region[SLIDES_HP];
    struct iv2d_setop ngon;

    struct iv2d_region const *vm_union;
    char ngon_name[128];

    struct slide slide[10];
    int count;
    int :32;
};

void slides_init(struct slides *s, int w, int h, double time);
void slides_fini(struct slides *s);
