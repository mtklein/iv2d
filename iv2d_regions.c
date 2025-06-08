#include "iv2d_regions.h"
#include <stdlib.h>
#include <string.h>

struct circle {
    struct iv2d_region region;
    float x,y,r, padding;
};
static iv circle(struct iv2d_region const *region, iv x, iv y) {
    struct circle const *c = (struct circle const*)region;
    return iv_sub(iv_add(iv_square(iv_sub(x, as_iv(c->x))),
                         iv_square(iv_sub(y, as_iv(c->y)))),
                  as_iv(c->r * c->r));
}
struct iv2d_region* iv2d_circle(float x, float y, float r) {
    struct circle *c = malloc(sizeof *c);
    *c = (struct circle){{circle}, .x=x, .y=y, .r=r};
    return &c->region;
}

struct capsule {
    struct iv2d_region region;
    float x0,y0,x1,y1,r, padding;
};
static iv capsule(struct iv2d_region const *region, iv x, iv y) {
    struct capsule const *c = (struct capsule const*)region;

    float const dx = c->x1 - c->x0,
                dy = c->y1 - c->y0;

    iv const px = iv_sub(x, as_iv(c->x0)),
             py = iv_sub(y, as_iv(c->y0));

    iv const t = iv_mul(iv_add(iv_mul(px, as_iv(dx)),
                               iv_mul(py, as_iv(dy))),
                        as_iv(1 / (dx*dx + dy*dy)));

    iv const h = iv_max(as_iv(0), iv_min(t, as_iv(1)));

    return iv_sub(iv_add(iv_square(iv_sub(px, iv_mul(h, as_iv(dx)))),
                         iv_square(iv_sub(py, iv_mul(h, as_iv(dy))))),
                  as_iv(c->r * c->r));
}
struct iv2d_region* iv2d_capsule(float x0, float y0, float x1, float y1, float r) {
    struct capsule *c = malloc(sizeof *c);
    *c = (struct capsule){{capsule}, .x0=x0, .y0=y0, .x1=x1, .y1=y1, .r=r};
    return &c->region;
}

struct binop {
    struct iv2d_region  op;
    int                 subregions,padding;
    struct iv2d_region *subregion[];
};
static iv union_(struct iv2d_region const *region, iv x, iv y) {
    struct binop const *b = (struct binop const*)region;
    iv v = as_iv(+1.0f/0);
    for (int i = 0; i < b->subregions; i++) {
        v = iv_min(v, b->subregion[i]->eval(b->subregion[i], x,y));
    }
    return v;
}
static iv intersect(struct iv2d_region const *region, iv x, iv y) {
    struct binop const *b = (struct binop const*)region;
    iv v = as_iv(-1.0f/0);
    for (int i = 0; i < b->subregions; i++) {
        v = iv_max(v, b->subregion[i]->eval(b->subregion[i], x,y));
    }
    return v;
}
struct iv2d_region* iv2d_union(struct iv2d_region const *subregion[], int subregions) {
    size_t const region_size = (size_t)subregions * sizeof *subregion;
    struct binop *b = malloc(sizeof *b + region_size);
    *b = (struct binop){{union_}, .subregions=subregions};
    memcpy(b->subregion, subregion, region_size);
    return &b->op;
}
struct iv2d_region* iv2d_intersect(struct iv2d_region const *subregion[], int subregions) {
    size_t const region_size = (size_t)subregions * sizeof *subregion;
    struct binop *b = malloc(sizeof *b + region_size);
    *b = (struct binop){{intersect}, .subregions=subregions};
    memcpy(b->subregion, subregion, region_size);
    return &b->op;
}

struct invert {
    struct iv2d_region        op;
    struct iv2d_region const *arg;
};
static iv invert(struct iv2d_region const *region, iv x, iv y) {
    struct invert const *inv = (struct invert const*)region;
    return iv_sub(as_iv(0), inv->arg->eval(inv->arg, x,y));
}
struct iv2d_region* iv2d_invert(struct iv2d_region const *arg) {
    struct invert *inv = malloc(sizeof *inv);
    *inv = (struct invert){{invert}, arg};
    return &inv->op;
}
