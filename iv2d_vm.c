#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

struct inst {
    union {
        struct {
            int spill                                              :  1;
            enum { IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQT,SQR,RET} op : 31;
        };
        int op_and_spill;
    };
    int      lhs,rhs;
    float        imm;
    float const *uni;
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
    b->inst[b->insts] = inst;
    return b->insts++;
}

builder* iv2d_builder(void) {
    builder *b = calloc(1, sizeof *b);
    push(b, (struct inst){.op=IMM});  // x
    push(b, (struct inst){.op=IMM});  // y
    return b;
}

int iv2d_x(builder *b) { (void)b; return 0; }
int iv2d_y(builder *b) { (void)b; return 1; }

int iv2d_imm(builder *b, float        imm) { return push(b, (struct inst){.op=IMM, .imm=imm}); }
int iv2d_uni(builder *b, float const *uni) { return push(b, (struct inst){.op=UNI, .uni=uni}); }

int iv2d_add(builder *b, int l, int r) { return push(b, (struct inst){.op=ADD, .lhs=l, .rhs=r}); }
int iv2d_sub(builder *b, int l, int r) { return push(b, (struct inst){.op=SUB, .lhs=l, .rhs=r}); }
int iv2d_mul(builder *b, int l, int r) { return push(b, (struct inst){.op=MUL, .lhs=l, .rhs=r}); }
int iv2d_min(builder *b, int l, int r) { return push(b, (struct inst){.op=MIN, .lhs=l, .rhs=r}); }
int iv2d_max(builder *b, int l, int r) { return push(b, (struct inst){.op=MAX, .lhs=l, .rhs=r}); }

int iv2d_abs   (builder *b, int v) { return push(b, (struct inst){.op=ABS, .rhs=v}); }
int iv2d_sqrt  (builder *b, int v) { return push(b, (struct inst){.op=SQT, .rhs=v}); }
int iv2d_square(builder *b, int v) { return push(b, (struct inst){.op=SQR, .rhs=v}); }

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct program {
    struct iv2d_region region;
    int         stack_slots, padding;
    struct inst inst[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    iv small[4096 / sizeof(iv)];
    iv *v = (p->stack_slots > len(small)) ? malloc((size_t)p->stack_slots * sizeof *v) : small;
    v[0] = x;
    iv rhs=y, *stack = v+1;  // the first inst will spill y to v[1].

    static void* const dispatch[] = {
        &&imm, &&imm_,
        &&uni, &&uni_,
        &&add, &&add_,
        &&sub, &&sub_,
        &&mul, &&mul_,
        &&min, &&min_,
        &&max, &&max_,
        &&abs, &&abs_,
        &&sqt, &&sqt_,
        &&sqr, &&sqr_,
        &&ret, &&ret_,
    };
    #define loop goto *dispatch[inst->op_and_spill]
    #define spill (*stack++ = rhs, rhs = v[inst->rhs])

    struct inst const *inst = p->inst;
    loop;

    imm_: spill;  imm: rhs = as_iv( inst->imm);          inst++; loop;
    uni_: spill;  uni: rhs = as_iv(*inst->uni);          inst++; loop;
    add_: spill;  add: rhs = iv_add(v[inst->lhs], rhs);  inst++; loop;
    sub_: spill;  sub: rhs = iv_sub(v[inst->lhs], rhs);  inst++; loop;
    mul_: spill;  mul: rhs = iv_mul(v[inst->lhs], rhs);  inst++; loop;
    min_: spill;  min: rhs = iv_min(v[inst->lhs], rhs);  inst++; loop;
    max_: spill;  max: rhs = iv_max(v[inst->lhs], rhs);  inst++; loop;
    abs_: spill;  abs: rhs = iv_abs(              rhs);  inst++; loop;
    sqt_: spill;  sqt: rhs = iv_sqrt(             rhs);  inst++; loop;
    sqr_: spill;  sqr: rhs = iv_square(           rhs);  inst++; loop;
    ret_: spill;  ret: if (v != small) { free(v); }       return rhs;

    #undef loop
    #undef spill
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    push(b, (struct inst){.op=RET, .rhs=ret});

    struct { int last_use, stack_slot; } *value = calloc((size_t)b->insts, sizeof *value);

    for (int i = 2; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;
        value[binst->lhs].last_use =
        value[binst->rhs].last_use = i;
    }

    struct program *p = malloc(sizeof *p + ((size_t)b->insts - 2) * sizeof *p->inst);
    *p = (struct program){.region={run_program}};
    value[0].stack_slot = p->stack_slots++;

    struct inst* inst = p->inst;
    for (int i = 2; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;

        _Bool const spill = value[i-1].last_use != i
                         || binst->lhs == i-1;
        if (spill) {
            value[i-1].stack_slot = p->stack_slots++;
        }

        *inst++ = (struct inst){
            .op    = binst->op,
            .spill = spill,
            .lhs   = value[binst->lhs].stack_slot,
            .rhs   = value[binst->rhs].stack_slot,
            .imm   = binst->imm,
            .uni   = binst->uni,
        };
    }

    free(value);
    free(b->inst);
    free(b);
    return &p->region;
}
