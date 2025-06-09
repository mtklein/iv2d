#pragma once

#include "iv2d.h"

struct iv2d_builder* iv2d_builder(void);
struct iv2d_region*  iv2d_ret(struct iv2d_builder*, int);  // free() when done

int iv2d_x(struct iv2d_builder*);
int iv2d_y(struct iv2d_builder*);

int iv2d_imm(struct iv2d_builder*, float);
int iv2d_uni(struct iv2d_builder*, float const*);

int iv2d_min(struct iv2d_builder*, int,int);
int iv2d_max(struct iv2d_builder*, int,int);
int iv2d_add(struct iv2d_builder*, int,int);
int iv2d_sub(struct iv2d_builder*, int,int);
int iv2d_mul(struct iv2d_builder*, int,int);
int iv2d_mad(struct iv2d_builder*, int,int,int);

int iv2d_abs   (struct iv2d_builder*, int);
int iv2d_sqrt  (struct iv2d_builder*, int);
int iv2d_square(struct iv2d_builder*, int);
