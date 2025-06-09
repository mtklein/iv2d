#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

typedef struct {
    enum {X,Y,IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQRT,SQUARE,RET} op;
    int          x,y;
    float        imm;
    float const *uni;

    //int uses, padding;
} binst;

typedef struct iv2d_builder {
    binst *inst;
    int    insts,padding;
} builder;

static _Bool is_pow2_or_zero(int n) {
    return 0 == (n & (n-1));
}

static int push(builder *b, binst inst) {
    if (is_pow2_or_zero(b->insts)) {
        b->inst = realloc(b->inst, (size_t)(b->insts ? 2*b->insts : 1) * sizeof *b->inst);
    }
    b->inst[b->insts] = inst;
    return b->insts++;
}

builder* iv2d_builder(void) {
    builder *b = calloc(1, sizeof *b);
    push(b, (binst){.op=X});
    push(b, (binst){.op=Y});
    return b;
}

int iv2d_x(builder *b) { (void)b; return 0; }
int iv2d_y(builder *b) { (void)b; return 1; }

int iv2d_imm   (builder *b, float        imm) { return push(b, (binst){IMM   , .imm=imm  }); }
int iv2d_uni   (builder *b, float const *uni) { return push(b, (binst){UNI   , .uni=uni  }); }
int iv2d_add   (builder *b, int x, int y    ) { return push(b, (binst){ADD   , .x=x, .y=y}); }
int iv2d_sub   (builder *b, int x, int y    ) { return push(b, (binst){SUB   , .x=x, .y=y}); }
int iv2d_mul   (builder *b, int x, int y    ) { return push(b, (binst){MUL   , .x=x, .y=y}); }
int iv2d_min   (builder *b, int x, int y    ) { return push(b, (binst){MIN   , .x=x, .y=y}); }
int iv2d_max   (builder *b, int x, int y    ) { return push(b, (binst){MAX   , .x=x, .y=y}); }
int iv2d_abs   (builder *b, int x           ) { return push(b, (binst){ABS   , .x=x      }); }
int iv2d_sqrt  (builder *b, int x           ) { return push(b, (binst){SQRT  , .x=x      }); }
int iv2d_square(builder *b, int x           ) { return push(b, (binst){SQUARE, .x=x      }); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct inst {
    iv (*op)(struct inst const *ip, iv *r, iv const *v);
    union {
        struct { int x,y; };
        float        imm;
        float const *uni;
    };
};

#define next return ip[1].op(ip+1, r+1, v)
#define op_(name) static iv op_##name(struct inst const *ip, iv *r, iv const *v)

op_(imm)    { *r = as_iv( ip->imm); next; }
op_(uni)    { *r = as_iv(*ip->uni); next; }
op_(add)    { *r = iv_add   (v[ip->x], v[ip->y]); next; }
op_(sub)    { *r = iv_sub   (v[ip->x], v[ip->y]); next; }
op_(mul)    { *r = iv_mul   (v[ip->x], v[ip->y]); next; }
op_(min)    { *r = iv_min   (v[ip->x], v[ip->y]); next; }
op_(max)    { *r = iv_max   (v[ip->x], v[ip->y]); next; }
op_(abs)    { *r = iv_abs   (v[ip->x]          ); next; }
op_(sqrt)   { *r = iv_sqrt  (v[ip->x]          ); next; }
op_(square) { *r = iv_square(v[ip->x]          ); next; }

op_(ret) { (void)r; return v[ip->x]; }

struct iv2d_program {
    struct iv2d_region region;
    int                vals, padding;
    struct inst        inst[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct iv2d_program const *p = (struct iv2d_program const*)region;

    iv small[128];
    iv *v = (p->vals > len(small)) ? malloc((size_t)p->vals * sizeof *v) : small;
    v[0] = x;
    v[1] = y;
    iv const ret = p->inst->op(p->inst,v+2,v);

    if (v != small) {
        free(v);
    }
    return ret;
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    // RET produces no value.
    int const vals = b->insts;
    push(b, (binst){RET, .x=ret});

    // X and Y are arguments, needing no instruction to produce.
    int insts = 0;
    for (int i = 0; i < b->insts; i++) {
        if (b->inst[i].op == X || b->inst[i].op == Y) {
            continue;
        }
        insts++;
    }

    struct iv2d_program *p = malloc(sizeof *p + (size_t)insts * sizeof *p->inst);
    *p = (struct iv2d_program){.region={run_program}, .vals=vals};

    struct inst* inst = p->inst;
    for (int i = 0; i < b->insts; i++) {
        binst const *binst = b->inst+i;
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wswitch-default"
        switch (binst->op) {
            case X:
            case Y: break;

            case IMM:    *inst++ = (struct inst){.op=op_imm   , .imm=binst->imm         }; break;
            case UNI:    *inst++ = (struct inst){.op=op_uni   , .uni=binst->uni         }; break;
            case ADD:    *inst++ = (struct inst){.op=op_add   , .x=binst->x, .y=binst->y}; break;
            case SUB:    *inst++ = (struct inst){.op=op_sub   , .x=binst->x, .y=binst->y}; break;
            case MUL:    *inst++ = (struct inst){.op=op_mul   , .x=binst->x, .y=binst->y}; break;
            case MIN:    *inst++ = (struct inst){.op=op_min   , .x=binst->x, .y=binst->y}; break;
            case MAX:    *inst++ = (struct inst){.op=op_max   , .x=binst->x, .y=binst->y}; break;
            case ABS:    *inst++ = (struct inst){.op=op_abs   , .x=binst->x             }; break;
            case SQRT:   *inst++ = (struct inst){.op=op_sqrt  , .x=binst->x             }; break;
            case SQUARE: *inst++ = (struct inst){.op=op_square, .x=binst->x             }; break;
            case RET:    *inst++ = (struct inst){.op=op_ret   , .x=binst->x             }; break;
        }
        #pragma clang diagnostic pop
    }
    //enum {X,Y,RET,IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQRT,SQUARE} op;

    free(b->inst);
    free(b);
    return &p->region;
}
