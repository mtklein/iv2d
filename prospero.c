#include "iv2d_vm.h"
#include "prospero.h"
#include <stdio.h>
#include <string.h>

static char const txt[] = {
    #embed "../prospero/prospero.vm"
    ,0
};

static _Bool starts_with(char const *haystack, char const *needle) {
    return 0 == memcmp(haystack, needle, strlen(needle));
}

struct iv2d_region* prospero_region(float w, float h) {
    int val[8000];
    struct iv2d_builder *b = iv2d_builder();

    unsigned id = 0;
    for (char const *c = txt; *c; c = strstr(c, "\n")+1) {
        if (*c == '#') {
            continue;
        }

        int skip;
        sscanf(c, "_%x %n", &id, &skip);
        c += skip;

        float imm;
        if (1 == sscanf(c, "const %f", &imm)) {
            val[id] = iv2d_imm(b, imm);
            continue;
        }
        if (starts_with(c, "var-x")) { // Scale [0,w) x to [-1,1].
            val[id] = iv2d_mad(b, iv2d_x(b), iv2d_imm(b, 2/(w-1)), iv2d_imm(b,-1));
            continue;
        }
        if (starts_with(c, "var-y")) { // Scale [0,h) y to [-1,1], flipped.
            val[id] = iv2d_mad(b, iv2d_y(b), iv2d_imm(b, -2/(h-1)), iv2d_imm(b,+1));
            continue;
        }

        unsigned l,r;
        if (2 == sscanf(c, "add _%x _%x", &l,&r)) {
            val[id] = iv2d_add(b, val[l], val[r]);
            continue;
        }
        if (2 == sscanf(c, "sub _%x _%x", &l,&r)) {
            val[id] = iv2d_sub(b, val[l], val[r]);
            continue;
        }
        if (2 == sscanf(c, "mul _%x _%x", &l,&r)) {
            val[id] = iv2d_mul(b, val[l], val[r]);
            continue;
        }
        if (2 == sscanf(c, "min _%x _%x", &l,&r)) {
            val[id] = iv2d_min(b, val[l], val[r]);
            continue;
        }
        if (2 == sscanf(c, "max _%x _%x", &l,&r)) {
            val[id] = iv2d_max(b, val[l], val[r]);
            continue;
        }
        if (1 == sscanf(c, "square _%x", &r)) {
            val[id] = iv2d_square(b, val[r]);
            continue;
        }
        if (1 == sscanf(c, "sqrt _%x", &r)) {
            val[id] = iv2d_sqrt(b, val[r]);
            continue;
        }
        if (1 == sscanf(c, "neg _%x", &r)) {
            val[id] = iv2d_sub(b, iv2d_imm(b,0), val[r]);
            continue;
        }
    }
    return iv2d_ret(b, val[id]);
}
