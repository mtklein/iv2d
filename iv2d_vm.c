#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

enum Op {RET,IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQRT,SQUARE};

typedef struct {
    enum Op      op;
    int          lhs,rhs;
    float        imm;
    float const *uni;
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
    push(b, (binst){.op=IMM});  // x
    push(b, (binst){.op=IMM});  // y
    return b;
}

int iv2d_x(builder *b) { (void)b; return 0; }
int iv2d_y(builder *b) { (void)b; return 1; }

int iv2d_imm(builder *b, float        imm) { return push(b, (binst){IMM, .imm=imm}); }
int iv2d_uni(builder *b, float const *uni) { return push(b, (binst){UNI, .uni=uni}); }

int iv2d_add(builder *b, int l, int r) { return push(b, (binst){ADD, .lhs=l, .rhs=r}); }
int iv2d_sub(builder *b, int l, int r) { return push(b, (binst){SUB, .lhs=l, .rhs=r}); }
int iv2d_mul(builder *b, int l, int r) { return push(b, (binst){MUL, .lhs=l, .rhs=r}); }
int iv2d_min(builder *b, int l, int r) { return push(b, (binst){MIN, .lhs=l, .rhs=r}); }
int iv2d_max(builder *b, int l, int r) { return push(b, (binst){MAX, .lhs=l, .rhs=r}); }

int iv2d_abs   (builder *b, int v) { return push(b, (binst){ABS   , .rhs=v}); }
int iv2d_sqrt  (builder *b, int v) { return push(b, (binst){SQRT  , .rhs=v}); }
int iv2d_square(builder *b, int v) { return push(b, (binst){SQUARE, .rhs=v}); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct inst {
    enum Op op;
    int     rhs;
    union {
        int          lhs;
        float        imm;
        float const *uni;
    };
};

struct program {
    struct iv2d_region region;
    int         slots, padding;
    struct inst inst[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    iv small[4096 / sizeof(iv)];
    iv *v = (p->slots > len(small)) ? malloc((size_t)p->slots * sizeof *v) : small;
    v[0] = x;
    iv rhs=y, *spill = v+1;

    static void* const dispatch[] = {
        [RET   ] = &&OP_RET,
        [IMM   ] = &&OP_IMM,
        [UNI   ] = &&OP_UNI,
        [ADD   ] = &&OP_ADD,
        [SUB   ] = &&OP_SUB,
        [MUL   ] = &&OP_MUL,
        [MIN   ] = &&OP_MIN,
        [MAX   ] = &&OP_MAX,
        [ABS   ] = &&OP_ABS,
        [SQRT  ] = &&OP_SQRT,
        [SQUARE] = &&OP_SQUARE,
    };

    for (struct inst const *inst = p->inst; ; inst++) {
        if (inst->rhs >= 0) {
            *spill++ = rhs;
            rhs = v[inst->rhs];
        }
        goto *dispatch[inst->op];

        OP_RET   : if (v != small) { free(v); } return rhs;
        OP_IMM   : rhs = as_iv( inst->imm);         continue;
        OP_UNI   : rhs = as_iv(*inst->uni);         continue;
        OP_ADD   : rhs = iv_add(v[inst->lhs], rhs); continue;
        OP_SUB   : rhs = iv_sub(v[inst->lhs], rhs); continue;
        OP_MUL   : rhs = iv_mul(v[inst->lhs], rhs); continue;
        OP_MIN   : rhs = iv_min(v[inst->lhs], rhs); continue;
        OP_MAX   : rhs = iv_max(v[inst->lhs], rhs); continue;
        OP_ABS   : rhs = iv_abs(              rhs); continue;
        OP_SQRT  : rhs = iv_sqrt(             rhs); continue;
        OP_SQUARE: rhs = iv_square(           rhs); continue;
    }
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    push(b, (binst){RET, .rhs=ret});

    struct {
        int last_use,slot;
    } *meta = calloc((size_t)b->insts, sizeof *meta);

    for (int i = 2; i < b->insts; i++) {
        binst const *binst = b->inst+i;
        meta[binst->lhs].last_use =
        meta[binst->rhs].last_use = i;
    }

    struct program *p = malloc(sizeof *p + ((size_t)b->insts - 2) * sizeof *p->inst);
    *p = (struct program){.region={run_program}, .slots=2};
    meta[0].slot = 0;
    meta[1].slot = 1;

    struct inst* inst = p->inst;
    _Bool rhs_in_reg = 0;
    for (int i = 2; i < b->insts; i++) {
        binst const *binst = b->inst+i;

        *inst = (struct inst){binst->op, .lhs=meta[binst->lhs].slot, .rhs=meta[binst->rhs].slot};
        if (binst->op == IMM) { inst->imm = binst->imm; }
        if (binst->op == UNI) { inst->uni = binst->uni; }
        if (rhs_in_reg) { inst->rhs = -1; }
        inst++;

        rhs_in_reg = (meta[i].last_use == i+1 && b->inst[i+1].lhs != i)
                  || binst->op == RET;
        if (!rhs_in_reg) {
            meta[i].slot = p->slots++;
        }
    }

    free(meta);
    free(b->inst);
    free(b);
    return &p->region;
}
