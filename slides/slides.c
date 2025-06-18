#include "../len.h"
#include "slides.h"

extern struct slide vm_union;

struct slide const *slide[] = {
    &vm_union,
};
int const slides = len(slide);
