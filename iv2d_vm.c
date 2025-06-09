#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

typedef struct {
    enum {X,Y,IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQRT,SQUARE,RET} op;
    int          x,y;
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
int iv2d_abs   (builder *b,        int y    ) { return push(b, (binst){ABS   ,       .y=y}); }
int iv2d_sqrt  (builder *b,        int y    ) { return push(b, (binst){SQRT  ,       .y=y}); }
int iv2d_square(builder *b,        int y    ) { return push(b, (binst){SQUARE,       .y=y}); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct inst {
    iv (*op)(struct inst const *ip, iv *m, iv const *v, iv h);
    union {
        struct { int x,y; };
        float        imm;
        float const *uni;
    };
};

#define op_(name) static iv name(struct inst const *ip, iv *m, iv const *v, iv r)
#define next return ip[1].op(ip+1, m+1, v, r)

op_(imm_m) { *m = as_iv(ip->imm); next; }
op_(imm_r) {  r = as_iv(ip->imm); next; }

op_(uni_m) { *m = as_iv(*ip->uni); next; }
op_(uni_r) {  r = as_iv(*ip->uni); next; }

op_(add_mm) { *m = iv_add(v[ip->x], v[ip->y]); next; }
op_(add_mr) { *m = iv_add(v[ip->x],        r); next; }
op_(add_rm) {  r = iv_add(v[ip->x], v[ip->y]); next; }
op_(add_rr) {  r = iv_add(v[ip->x],        r); next; }

op_(sub_mm) { *m = iv_sub(v[ip->x], v[ip->y]); next; }
op_(sub_mr) { *m = iv_sub(v[ip->x],        r); next; }
op_(sub_rm) {  r = iv_sub(v[ip->x], v[ip->y]); next; }
op_(sub_rr) {  r = iv_sub(v[ip->x],        r); next; }

op_(mul_mm) { *m = iv_mul(v[ip->x], v[ip->y]); next; }
op_(mul_mr) { *m = iv_mul(v[ip->x],        r); next; }
op_(mul_rm) {  r = iv_mul(v[ip->x], v[ip->y]); next; }
op_(mul_rr) {  r = iv_mul(v[ip->x],        r); next; }

op_(min_mm) { *m = iv_min(v[ip->x], v[ip->y]); next; }
op_(min_mr) { *m = iv_min(v[ip->x],        r); next; }
op_(min_rm) {  r = iv_min(v[ip->x], v[ip->y]); next; }
op_(min_rr) {  r = iv_min(v[ip->x],        r); next; }

op_(max_mm) { *m = iv_max(v[ip->x], v[ip->y]); next; }
op_(max_mr) { *m = iv_max(v[ip->x],        r); next; }
op_(max_rm) {  r = iv_max(v[ip->x], v[ip->y]); next; }
op_(max_rr) {  r = iv_max(v[ip->x],        r); next; }

op_(abs_mm) { *m = iv_abs(v[ip->y]); next; }
op_(abs_mr) { *m = iv_abs(       r); next; }
op_(abs_rm) {  r = iv_abs(v[ip->y]); next; }
op_(abs_rr) {  r = iv_abs(       r); next; }

op_(sqrt_mm) { *m = iv_sqrt(v[ip->y]); next; }
op_(sqrt_mr) { *m = iv_sqrt(       r); next; }
op_(sqrt_rm) {  r = iv_sqrt(v[ip->y]); next; }
op_(sqrt_rr) {  r = iv_sqrt(       r); next; }

op_(square_mm) { *m = iv_square(v[ip->y]); next; }
op_(square_mr) { *m = iv_square(       r); next; }
op_(square_rm) {  r = iv_square(v[ip->y]); next; }
op_(square_rr) {  r = iv_square(       r); next; }

op_(ret_m) { (void)m; (void)r;           return v[ip->y]; }
op_(ret_r) { (void)m; (void)v; (void)ip; return        r; }

// mm,mr,rm,rr
static iv (*op_fn[][4])(struct inst const*, iv*, iv const*, iv) = {
    [X] = {NULL,NULL,NULL,NULL},
    [Y] = {NULL,NULL,NULL,NULL},

    [IMM] = {imm_m,imm_m, imm_r,imm_r},  // Splatting to memory or register?
    [UNI] = {uni_m,uni_m, uni_r,uni_r},

    [ADD   ] = {   add_mm,    add_mr,    add_rm,    add_rr},
    [SUB   ] = {   sub_mm,    sub_mr,    sub_rm,    sub_rr},
    [MUL   ] = {   mul_mm,    mul_mr,    mul_rm,    mul_rr},
    [MIN   ] = {   min_mm,    min_mr,    min_rm,    min_rr},
    [MAX   ] = {   max_mm,    max_mr,    max_rm,    max_rr},
    [ABS   ] = {   abs_mm,    abs_mr,    abs_rm,    abs_rr},
    [SQRT  ] = {  sqrt_mm,   sqrt_mr,   sqrt_rm,   sqrt_rr},
    [SQUARE] = {square_mm, square_mr, square_rm, square_rr},

    [RET] = {ret_m,ret_r, ret_m,ret_r},  // Are we returning the hot register or v[ip->y]?
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
    iv const ret = p->inst->op(p->inst,v+2,v, as_iv(0));

    if (v != small) {
        free(v);
    }
    return ret;
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    // RET produces no value.
    int const vals = b->insts;
    push(b, (binst){RET, .y=ret, .last_use=vals});

    for (int i = 0; i < b->insts; i++) {
        binst const *binst = b->inst+i;
        b->inst[binst->x].last_use = i;
        b->inst[binst->y].last_use = i;
    }

    // X and Y are arguments, needing no instruction to produce.
    int insts = 0;
    for (binst const *binst = b->inst; binst < b->inst+b->insts; binst++) {
        if (binst->op == X || binst->op == Y) {
            continue;
        }
        insts++;
    }

    struct program *p = malloc(sizeof *p + (size_t)insts * sizeof *p->inst);
    *p = (struct program){.region={run_program}, .vals=vals};

    struct inst* inst = p->inst;
    _Bool arg_in_reg = 0;
    for (int i = 0; i < b->insts; i++) {
        binst const *binst = b->inst+i;

        _Bool const leave_in_reg = binst->last_use == i+1
                                  && b->inst[i+1].y == i
                                  && b->inst[i+1].x != i;
        // op_fn array order is mm,mr,rm,rr
        //   leave_in_reg=0, arg_in_reg=0 -> mm (0)
        //   leave_in_reg=0, arg_in_reg=1 -> mr (1)
        //   leave_in_reg=1, arg_in_reg=0 -> rm (2)
        //   leave_in_reg=1, arg_in_reg=1 -> rr (3)

        iv (*op)(struct inst const*, iv*, iv const*, iv)
            = op_fn[binst->op][2*(int)leave_in_reg + (int)arg_in_reg];

        if (!op) {
            continue;
        }

        *inst = (struct inst){.op=op, .x=binst->x, .y=binst->y};
        if (binst->op == IMM) { inst->imm = binst->imm; }
        if (binst->op == UNI) { inst->uni = binst->uni; }
        inst++;
        arg_in_reg = leave_in_reg;
    }

    free(b->inst);
    free(b);
    return &p->region;
}
