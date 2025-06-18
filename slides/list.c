#include "../slide.h"

extern struct slide union_slide;
extern struct slide intersect_slide;
extern struct slide difference_slide;
extern struct slide capsule_slide;
extern struct slide halfplane_slide;
extern struct slide ngon_slide;
extern struct slide vm_union_slide;
extern struct slide prospero_slide;

struct slide *slides[] = {
    &union_slide,
    &intersect_slide,
    &difference_slide,
    &capsule_slide,
    &halfplane_slide,
    &ngon_slide,
    &vm_union_slide,
    &prospero_slide,
};
int slide_count = sizeof(slides)/sizeof(slides[0]);
