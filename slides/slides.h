#pragma once

struct slide {
    char const *name;
    struct iv2d_region const* (*create)(float const *w, float const *h, float const *t);
    void (*cleanup)(struct iv2d_region const*);
};

extern struct slide const* slide[];
extern int const           slides;
