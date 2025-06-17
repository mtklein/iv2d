#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>
#include <string.h>

struct inst {
    enum { RET,IMM,UNI,X,Y,ABS,SQT,SQR,INV,ADD,SUB,MUL,MIN,MAX} op;
    int          lhs,rhs;
    float        imm;
    float const *ptr;
};

typedef struct iv2d_builder {
    struct inst *inst;
    int          insts,padding;
} builder;

static _Bool is_pow2_or_zero(int n) {
    return 0 == (n & (n-1));
}
static int push(builder *b, struct inst inst) {
    if (is_pow2_or_zero(b->insts)) {
        b->inst = realloc(b->inst, (size_t)(b->insts ? 2*b->insts : 1) * sizeof *b->inst);
    }
    int const id = b->insts++;
    b->inst[id] = inst;
    return id;
}

builder* iv2d_builder(void) {
    builder *b = calloc(1, sizeof *b);
    return b;
}

int iv2d_x(builder *b) { return push(b, (struct inst){.op=X}); }
int iv2d_y(builder *b) { return push(b, (struct inst){.op=Y}); }

int iv2d_imm(builder *b, float        imm) { return push(b, (struct inst){.op=IMM, .imm=imm}); }
int iv2d_uni(builder *b, float const *ptr) { return push(b, (struct inst){.op=UNI, .ptr=ptr}); }

int iv2d_abs   (builder *b, int v) { return push(b, (struct inst){.op=ABS, .rhs=v}); }
int iv2d_sqrt  (builder *b, int v) { return push(b, (struct inst){.op=SQT, .rhs=v}); }
int iv2d_square(builder *b, int v) { return push(b, (struct inst){.op=SQR, .rhs=v}); }
int iv2d_inv   (builder *b, int v) { return push(b, (struct inst){.op=INV, .rhs=v}); }

int iv2d_sub(builder *b, int l, int r) { return push(b, (struct inst){.op=SUB, .lhs=l, .rhs=r}); }
int iv2d_add(builder *b, int l, int r) { return push(b, (struct inst){.op=ADD, .lhs=l, .rhs=r}); }
int iv2d_mul(builder *b, int l, int r) { return push(b, (struct inst){.op=MUL, .lhs=l, .rhs=r}); }
int iv2d_min(builder *b, int l, int r) { return push(b, (struct inst){.op=MIN, .lhs=l, .rhs=r}); }
int iv2d_max(builder *b, int l, int r) { return push(b, (struct inst){.op=MAX, .lhs=l, .rhs=r}); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct program {
    struct iv2d_region region;
    int         vals, imms;
    struct inst inst[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    _Thread_local static struct program const *active_program;
    _Thread_local static iv *v;
    if (active_program != p) {
        active_program  = p;
        v = realloc(v, (size_t)p->vals * sizeof *v);
        for (int i = 0; i < p->imms; i++) {
            v[i] = as_iv(p->inst[i].imm);
        }
    }

    iv *next = v+p->imms;
    for (struct inst const *inst = p->inst+p->imms; ; inst++) {
        #pragma clang diagnostic ignored "-Wswitch-default"
        switch (inst->op) {
            case IMM: __builtin_unreachable();

            case UNI: *next++ = as_iv(*inst->ptr);                  break;
            case X:   *next++ = x;                                  break;
            case Y:   *next++ = y;                                  break;
            case ABS: *next++ = iv_abs   (           v[inst->rhs]); break;
            case SQT: *next++ = iv_sqrt  (           v[inst->rhs]); break;
            case SQR: *next++ = iv_square(           v[inst->rhs]); break;
            case INV: *next++ = iv_inv   (           v[inst->rhs]); break;
            case ADD: *next++ = iv_add(v[inst->lhs], v[inst->rhs]); break;
            case SUB: *next++ = iv_sub(v[inst->lhs], v[inst->rhs]); break;
            case MUL: *next++ = iv_mul(v[inst->lhs], v[inst->rhs]); break;
            case MIN: *next++ = iv_min(v[inst->lhs], v[inst->rhs]); break;
            case MAX: *next++ = iv_max(v[inst->lhs], v[inst->rhs]); break;

            case RET: return v[inst->rhs];
        }
    }
}

struct iv2d_region const* iv2d_ret(builder *b, int ret) {
    push(b, (struct inst){.op=RET, .rhs=ret});

    struct program *p = malloc(sizeof *p + (size_t)b->insts * sizeof *p->inst);
    *p = (struct program){.region={run_program}};

    int *index = calloc((size_t)b->insts, sizeof *index);
    for (int imm = 2; imm --> 0;)
    for (int i = 0; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;
        struct inst       *pinst = p->inst+p->vals;
        if (imm == (binst->op == IMM)) {
            p->imms += (binst->op == IMM);
            *pinst = *binst;
            pinst->lhs = index[binst->lhs];
            pinst->rhs = index[binst->rhs];
            index[i] = p->vals++;
        }
    }

    free(index);
    free(b->inst);
    free(b);
    return &p->region;
}
