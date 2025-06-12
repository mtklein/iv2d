#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

struct inst {
    union {
        struct {
            _Bool spill                                                :  1;
            enum { X,Y,IMM,UNI,RET,ABS,SQT,SQR,ADD,SUB,MUL,MIN,MAX} op : 31;
        };
        int op_and_spill;
    };
    int rhs;
    union {
        int          lhs;
        float        imm;
        float const *ptr;
    };
};

static _Bool uses_lhs(struct inst const *inst) { return inst->op >= ADD; }
static _Bool uses_rhs(struct inst const *inst) { return inst->op >= RET; }

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
    b->inst[b->insts] = inst;
    return b->insts++;
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

int iv2d_add(builder *b, int l, int r) { return push(b, (struct inst){.op=ADD, .lhs=l, .rhs=r}); }
int iv2d_sub(builder *b, int l, int r) { return push(b, (struct inst){.op=SUB, .lhs=l, .rhs=r}); }
int iv2d_mul(builder *b, int l, int r) { return push(b, (struct inst){.op=MUL, .lhs=l, .rhs=r}); }
int iv2d_min(builder *b, int l, int r) { return push(b, (struct inst){.op=MIN, .lhs=l, .rhs=r}); }
int iv2d_max(builder *b, int l, int r) { return push(b, (struct inst){.op=MAX, .lhs=l, .rhs=r}); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct program {
    struct iv2d_region region;
    struct inst inst[];
};

static iv run_program(struct iv2d_region const *region, void *scratch, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    static void* const dispatch[] = {
        &&opx, &&opx_,
        &&opy, &&opy_,
        &&imm, &&imm_,
        &&uni, &&uni_,
        &&ret, &&ret_,
        &&abs, &&abs_,
        &&sqt, &&sqt_,
        &&sqr, &&sqr_,
        &&add, &&add_,
        &&sub, &&sub_,
        &&mul, &&mul_,
        &&min, &&min_,
        &&max, &&max_,
    };
    #define loop goto *dispatch[inst->op_and_spill]
    #define spill (*spill_slot++ = rhs, rhs = v[inst->rhs])
    iv *v = scratch, *spill_slot = v, rhs = as_iv(0);
    struct inst const *inst = p->inst;
    loop;

    opx_: spill;  opx: rhs = x;                          inst++; loop;
    opy_: spill;  opy: rhs = y;                          inst++; loop;
    imm_: spill;  imm: rhs = as_iv( inst->imm);          inst++; loop;
    uni_: spill;  uni: rhs = as_iv(*inst->ptr);          inst++; loop;
    abs_: spill;  abs: rhs = iv_abs(              rhs);  inst++; loop;
    sqt_: spill;  sqt: rhs = iv_sqrt(             rhs);  inst++; loop;
    sqr_: spill;  sqr: rhs = iv_square(           rhs);  inst++; loop;
    add_: spill;  add: rhs = iv_add(v[inst->lhs], rhs);  inst++; loop;
    sub_: spill;  sub: rhs = iv_sub(v[inst->lhs], rhs);  inst++; loop;
    mul_: spill;  mul: rhs = iv_mul(v[inst->lhs], rhs);  inst++; loop;
    min_: spill;  min: rhs = iv_min(v[inst->lhs], rhs);  inst++; loop;
    max_: spill;  max: rhs = iv_max(v[inst->lhs], rhs);  inst++; loop;

    ret_: rhs = v[inst->rhs]; ret: return rhs;
    #undef loop
    #undef spill
}

struct iv2d_region const* iv2d_ret(builder *b, int ret) {
    push(b, (struct inst){.op=RET, .rhs=ret});

    struct { int last_use, spill_slot; } *value = calloc((size_t)b->insts, sizeof *value);

    for (int i = 0; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;
        if (uses_lhs(binst)) { value[binst->lhs].last_use = i; }
        if (uses_rhs(binst)) { value[binst->rhs].last_use = i; }
    }

    struct program *p = malloc(sizeof *p + (size_t)b->insts * sizeof *p->inst);
    *p = (struct program){.region={run_program, 0}};

    int spills = 0;
    for (int i = 0; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;

        _Bool const spill = (i > 0)
                         && (value[i-1].last_use > i || (uses_lhs(binst) && binst->lhs == i-1));
        if (spill) {
            value[i-1].spill_slot = spills++;
        }

        p->inst[i] = (struct inst){
            .op    = binst->op,
            .spill = spill,
            .rhs   = value[binst->rhs].spill_slot,
            .ptr   = binst->ptr,
        };
        if (uses_lhs(binst)) { p->inst[i].lhs = value[binst->lhs].spill_slot; }
    }
    p->region.scratch += sizeof(iv) * (size_t)spills;

    free(value);
    free(b->inst);
    free(b);
    return &p->region;
}
