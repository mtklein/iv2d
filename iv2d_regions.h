#pragma once

#include "iv2d.h"

// All iv2d_regions from this file can be cleaned up with free(),
// and none take ownership of their iv2d_region arguments.

struct iv2d_region* iv2d_circle(float x, float y, float r);
struct iv2d_region* iv2d_capsule(float x0, float y0, float x1, float y1, float r);

struct iv2d_region* iv2d_union    (struct iv2d_region const *subregion[], int subregions);
struct iv2d_region* iv2d_intersect(struct iv2d_region const *subregion[], int subregions);

struct iv2d_region* iv2d_invert(struct iv2d_region const*);
