#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

typedef struct {
    enum {PHONY,IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQRT,SQUARE,RET} op;
    int          lhs,rhs;
    float        imm;
    float const *uni;

    int last_use, padding;
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
    push(b, (binst){.op=PHONY});  // x
    push(b, (binst){.op=PHONY});  // y
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
    iv (*op)(struct inst const *ip, iv *m, iv const *v, iv h);
    union {
        struct { int lhs,rhs; };
        float        imm;
        float const *uni;
    };
};

#define op_(name) static iv name(struct inst const *ip, iv *m, iv const *v, iv reg)
#define next return ip[1].op(ip+1, m+1, v, reg)

// Ops are split up to 4 ways,
//   m/r - write to *m or reg?
//   v/r - read rhs from v or reg?

op_(imm_m) {  *m = as_iv(ip->imm); next; }
op_(imm_r) { reg = as_iv(ip->imm); next; }

op_(uni_m) {  *m = as_iv(*ip->uni); next; }
op_(uni_r) { reg = as_iv(*ip->uni); next; }

op_(add_mv) {  *m = iv_add(v[ip->lhs], v[ip->rhs]); next; }
op_(add_mr) {  *m = iv_add(v[ip->lhs], reg       ); next; }
op_(add_rv) { reg = iv_add(v[ip->lhs], v[ip->rhs]); next; }
op_(add_rr) { reg = iv_add(v[ip->lhs], reg       ); next; }

op_(sub_mv) {  *m = iv_sub(v[ip->lhs], v[ip->rhs]); next; }
op_(sub_mr) {  *m = iv_sub(v[ip->lhs], reg       ); next; }
op_(sub_rv) { reg = iv_sub(v[ip->lhs], v[ip->rhs]); next; }
op_(sub_rr) { reg = iv_sub(v[ip->lhs], reg       ); next; }

op_(mul_mv) {  *m = iv_mul(v[ip->lhs], v[ip->rhs]); next; }
op_(mul_mr) {  *m = iv_mul(v[ip->lhs], reg       ); next; }
op_(mul_rv) { reg = iv_mul(v[ip->lhs], v[ip->rhs]); next; }
op_(mul_rr) { reg = iv_mul(v[ip->lhs], reg       ); next; }

op_(min_mv) {  *m = iv_min(v[ip->lhs], v[ip->rhs]); next; }
op_(min_mr) {  *m = iv_min(v[ip->lhs], reg       ); next; }
op_(min_rv) { reg = iv_min(v[ip->lhs], v[ip->rhs]); next; }
op_(min_rr) { reg = iv_min(v[ip->lhs], reg       ); next; }

op_(max_mv) {  *m = iv_max(v[ip->lhs], v[ip->rhs]); next; }
op_(max_mr) {  *m = iv_max(v[ip->lhs], reg       ); next; }
op_(max_rv) { reg = iv_max(v[ip->lhs], v[ip->rhs]); next; }
op_(max_rr) { reg = iv_max(v[ip->lhs], reg       ); next; }

op_(abs_mv) {  *m = iv_abs(v[ip->rhs]); next; }
op_(abs_mr) {  *m = iv_abs(reg       ); next; }
op_(abs_rv) { reg = iv_abs(v[ip->rhs]); next; }
op_(abs_rr) { reg = iv_abs(reg       ); next; }

op_(sqrt_mv) {  *m = iv_sqrt(v[ip->rhs]); next; }
op_(sqrt_mr) {  *m = iv_sqrt(reg       ); next; }
op_(sqrt_rv) { reg = iv_sqrt(v[ip->rhs]); next; }
op_(sqrt_rr) { reg = iv_sqrt(reg       ); next; }

op_(square_mv) {  *m = iv_square(v[ip->rhs]); next; }
op_(square_mr) {  *m = iv_square(reg       ); next; }
op_(square_rv) { reg = iv_square(v[ip->rhs]); next; }
op_(square_rr) { reg = iv_square(reg       ); next; }

op_(ret_v) {           (void)m; (void)reg; return v[ip->rhs]; }
op_(ret_r) { (void)ip; (void)m; (void)v;   return reg       ; }

static iv (*op_fn[][4])(struct inst const *ip, iv *m, iv const *v, iv reg) = {
    [IMM] = {imm_m,imm_m, imm_r,imm_r},
    [UNI] = {uni_m,uni_m, uni_r,uni_r},

    [ADD   ] = {   add_mv,    add_mr,    add_rv,    add_rr},
    [SUB   ] = {   sub_mv,    sub_mr,    sub_rv,    sub_rr},
    [MUL   ] = {   mul_mv,    mul_mr,    mul_rv,    mul_rr},
    [MIN   ] = {   min_mv,    min_mr,    min_rv,    min_rr},
    [MAX   ] = {   max_mv,    max_mr,    max_rv,    max_rr},
    [ABS   ] = {   abs_mv,    abs_mr,    abs_rv,    abs_rr},
    [SQRT  ] = {  sqrt_mv,   sqrt_mr,   sqrt_rv,   sqrt_rr},
    [SQUARE] = {square_mv, square_mr, square_rv, square_rr},

    [RET] = {ret_v,ret_r, ret_v,ret_r},
};

struct program {
    struct iv2d_region region;
    int                vals, padding;
    struct inst        inst[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    iv small[128];
    iv *v = (p->vals > len(small)) ? malloc((size_t)p->vals * sizeof *v) : small;
    v[0] = x;
    v[1] = y;
    iv const ret = p->inst->op(p->inst,v+2,v, x/*anything will do, this is cheapest*/);

    if (v != small) {
        free(v);
    }
    return ret;
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    push(b, (binst){RET, .rhs=ret});

    for (int i = 2; i < b->insts; i++) {
        binst const *binst = b->inst+i;
        b->inst[binst->lhs].last_use = i;
        b->inst[binst->rhs].last_use = i;
    }

    struct program *p = malloc(sizeof *p + (size_t)b->insts * sizeof *p->inst);
    *p = (struct program){.region={run_program}, .vals=b->insts};

    struct inst* inst = p->inst;
    _Bool rhs_in_reg = 0;
    for (int i = 2; i < b->insts; i++) {
        binst const *binst = b->inst+i;

        _Bool const write_to_reg = binst->last_use == i+1
                                && b->inst[i+1].lhs != i
                                && b->inst[i+1].rhs == i;
        // op_fn array order is
        //   write_to_reg=0, rhs_in_reg=0 -> 0 (mv)
        //   write_to_reg=0, rhs_in_reg=1 -> 1 (mr)
        //   write_to_reg=1, rhs_in_reg=0 -> 2 (rv)
        //   write_to_reg=1, rhs_in_reg=1 -> 3 (rr)

        iv (*op)(struct inst const*, iv*, iv const*, iv)
            = op_fn[binst->op][2*(int)write_to_reg + (int)rhs_in_reg];

        *inst = (struct inst){.op=op, .lhs=binst->lhs, .rhs=binst->rhs};
        if (binst->op == IMM) { inst->imm = binst->imm; }
        if (binst->op == UNI) { inst->uni = binst->uni; }
        inst++;

        rhs_in_reg = write_to_reg;
    }

    free(b->inst);
    free(b);
    return &p->region;
}
