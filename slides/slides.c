#include "../len.h"
#include "slides.h"

extern struct slide union_slide;
extern struct slide intersect_slide;
extern struct slide difference_slide;
extern struct slide capsule_slide;
extern struct slide halfplane_slide;
extern struct slide ngon_slide;
extern struct slide vm_union;

struct slide const *slide[] = {
    &union_slide,
    &intersect_slide,
    &difference_slide,
    &capsule_slide,
    &halfplane_slide,
    &ngon_slide,
    &vm_union,
}; 
int const slides = len(slide);
