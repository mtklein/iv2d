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
int iv2d_abs   (builder *b, int x           ) { return push(b, (binst){ABS   , .x=x      }); }
int iv2d_sqrt  (builder *b, int x           ) { return push(b, (binst){SQRT  , .x=x      }); }
int iv2d_square(builder *b, int x           ) { return push(b, (binst){SQUARE, .x=x      }); }

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

op_(add_mx) { *m = iv_add(v[ip->x], v[ip->y]); next; }
op_(add_mr) { *m = iv_add(       r, v[ip->y]); next; }
op_(add_rx) {  r = iv_add(v[ip->x], v[ip->y]); next; }
op_(add_rr) {  r = iv_add(       r, v[ip->y]); next; }

op_(sub_mx) { *m = iv_sub(v[ip->x], v[ip->y]); next; }
op_(sub_mr) { *m = iv_sub(       r, v[ip->y]); next; }
op_(sub_rx) {  r = iv_sub(v[ip->x], v[ip->y]); next; }
op_(sub_rr) {  r = iv_sub(       r, v[ip->y]); next; }

op_(mul_mx) { *m = iv_mul(v[ip->x], v[ip->y]); next; }
op_(mul_mr) { *m = iv_mul(       r, v[ip->y]); next; }
op_(mul_rx) {  r = iv_mul(v[ip->x], v[ip->y]); next; }
op_(mul_rr) {  r = iv_mul(       r, v[ip->y]); next; }

op_(min_mx) { *m = iv_min(v[ip->x], v[ip->y]); next; }
op_(min_mr) { *m = iv_min(       r, v[ip->y]); next; }
op_(min_rx) {  r = iv_min(v[ip->x], v[ip->y]); next; }
op_(min_rr) {  r = iv_min(       r, v[ip->y]); next; }

op_(max_mx) { *m = iv_max(v[ip->x], v[ip->y]); next; }
op_(max_mr) { *m = iv_max(       r, v[ip->y]); next; }
op_(max_rx) {  r = iv_max(v[ip->x], v[ip->y]); next; }
op_(max_rr) {  r = iv_max(       r, v[ip->y]); next; }

op_(abs_mx) { *m = iv_abs(v[ip->x]); next; }
op_(abs_mr) { *m = iv_abs(       r); next; }
op_(abs_rx) {  r = iv_abs(v[ip->x]); next; }
op_(abs_rr) {  r = iv_abs(       r); next; }

op_(sqrt_mx) { *m = iv_sqrt(v[ip->x]); next; }
op_(sqrt_mr) { *m = iv_sqrt(       r); next; }
op_(sqrt_rx) {  r = iv_sqrt(v[ip->x]); next; }
op_(sqrt_rr) {  r = iv_sqrt(       r); next; }

op_(square_mx) { *m = iv_square(v[ip->x]); next; }
op_(square_mr) { *m = iv_square(       r); next; }
op_(square_rx) {  r = iv_square(v[ip->x]); next; }
op_(square_rr) {  r = iv_square(       r); next; }

op_(ret_x) { (void)m; (void)r;           return v[ip->x]; }
op_(ret_r) { (void)m; (void)v; (void)ip; return        r; }

// mx,mr,rx,rr
static iv (*op_fn[][4])(struct inst const*, iv*, iv const*, iv) = {
    [X] = {NULL,NULL,NULL,NULL},
    [Y] = {NULL,NULL,NULL,NULL},

    [IMM] = {imm_m,imm_m, imm_r,imm_r},  // Splatting to memory or register?
    [UNI] = {uni_m,uni_m, uni_r,uni_r},

    [ADD   ] = {   add_mx,    add_mr,    add_rx,    add_rr},
    [SUB   ] = {   sub_mx,    sub_mr,    sub_rx,    sub_rr},
    [MUL   ] = {   mul_mx,    mul_mr,    mul_rx,    mul_rr},
    [MIN   ] = {   min_mx,    min_mr,    min_rx,    min_rr},
    [MAX   ] = {   max_mx,    max_mr,    max_rx,    max_rr},
    [ABS   ] = {   abs_mx,    abs_mr,    abs_rx,    abs_rr},
    [SQRT  ] = {  sqrt_mx,   sqrt_mr,   sqrt_rx,   sqrt_rr},
    [SQUARE] = {square_mx, square_mr, square_rx, square_rr},

    [RET] = {ret_x,ret_r, ret_x,ret_r},  // Are we returning the hot register or v[ip->x]?
};

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
    iv const ret = p->inst->op(p->inst,v+2,v, as_iv(0));

    if (v != small) {
        free(v);
    }
    return ret;
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    // RET produces no value.
    int const vals = b->insts;
    push(b, (binst){RET, .x=ret, .last_use=vals});

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

    struct iv2d_program *p = malloc(sizeof *p + (size_t)insts * sizeof *p->inst);
    *p = (struct iv2d_program){.region={run_program}, .vals=vals};

    struct inst* inst = p->inst;
    _Bool x_in_reg = 0;
    for (int i = 0; i < b->insts; i++) {
        binst const *binst = b->inst+i;

        _Bool const leave_x_in_reg = binst->last_use == i+1
                                  && b->inst[i+1].x == i
                                  && b->inst[i+1].y != i;
        // op_fn array order is mx,mr,rx,rr
        //   leave_x_in_reg=0, x_in_reg=0 -> mx (0)
        //   leave_x_in_reg=0, x_in_reg=1 -> mr (1)
        //   leave_x_in_reg=1, x_in_reg=0 -> rx (2)
        //   leave_x_in_reg=1, x_in_reg=1 -> rr (3)

        iv (*op)(struct inst const*, iv*, iv const*, iv)
            = op_fn[binst->op][2*(int)leave_x_in_reg + (int)x_in_reg];

        if (!op) {
            continue;
        }

        *inst = (struct inst){.op=op, .x=binst->x, .y=binst->y};
        if (binst->op == IMM) { inst->imm = binst->imm; }
        if (binst->op == UNI) { inst->uni = binst->uni; }
        inst++;
        x_in_reg = leave_x_in_reg;
    }

    free(b->inst);
    free(b);
    return &p->region;
}
