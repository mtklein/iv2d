#pragma once

struct slide {
    char const *name;
    struct iv2d_region const* (*create)(float w, float h, float t);
    void (*cleanup)(struct iv2d_region const*);
};

extern struct slide const* slide[];
extern int const           slides;
