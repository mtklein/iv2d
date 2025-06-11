#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>

struct inst {
    enum Op {IMM,UNI,ADD,SUB,MUL,MIN,MAX,ABS,SQT,SQR,RET} op;
    int          lhs,rhs;
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

int iv2d_imm(builder *b, float        imm) { return push(b, (struct inst){IMM, .imm=imm}); }
int iv2d_uni(builder *b, float const *uni) { return push(b, (struct inst){UNI, .uni=uni}); }

int iv2d_add(builder *b, int l, int r) { return push(b, (struct inst){ADD, .lhs=l, .rhs=r}); }
int iv2d_sub(builder *b, int l, int r) { return push(b, (struct inst){SUB, .lhs=l, .rhs=r}); }
int iv2d_mul(builder *b, int l, int r) { return push(b, (struct inst){MUL, .lhs=l, .rhs=r}); }
int iv2d_min(builder *b, int l, int r) { return push(b, (struct inst){MIN, .lhs=l, .rhs=r}); }
int iv2d_max(builder *b, int l, int r) { return push(b, (struct inst){MAX, .lhs=l, .rhs=r}); }

int iv2d_abs   (builder *b, int v) { return push(b, (struct inst){ABS, .rhs=v}); }
int iv2d_sqrt  (builder *b, int v) { return push(b, (struct inst){SQT, .rhs=v}); }
int iv2d_square(builder *b, int v) { return push(b, (struct inst){SQR, .rhs=v}); }

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

    // TODO: try separate fall-through spill targets again; surprised that wasn't any faster.
    static void* const dispatch[] = {
        &&IMM_, &&UNI_, &&ADD_, &&SUB_, &&MUL_, &&MIN_, &&MAX_, &&ABS_, &&SQT_, &&SQR_, &&RET_
    };
    #define loop goto *dispatch[inst->op]
    #define maybe_spill if (inst->rhs >= 0) (*stack++ = rhs, rhs = v[inst->rhs])

    struct inst const *inst = p->inst;
    loop;

    IMM_: maybe_spill;   rhs = as_iv( inst->imm);           inst++; loop;
    UNI_: maybe_spill;   rhs = as_iv(*inst->uni);           inst++; loop;
    ADD_: maybe_spill;   rhs = iv_add(v[inst->lhs], rhs);   inst++; loop;
    SUB_: maybe_spill;   rhs = iv_sub(v[inst->lhs], rhs);   inst++; loop;
    MUL_: maybe_spill;   rhs = iv_mul(v[inst->lhs], rhs);   inst++; loop;
    MIN_: maybe_spill;   rhs = iv_min(v[inst->lhs], rhs);   inst++; loop;
    MAX_: maybe_spill;   rhs = iv_max(v[inst->lhs], rhs);   inst++; loop;
    ABS_: maybe_spill;   rhs = iv_abs(              rhs);   inst++; loop;
    SQT_: maybe_spill;   rhs = iv_sqrt(             rhs);   inst++; loop;
    SQR_: maybe_spill;   rhs = iv_square(           rhs);   inst++; loop;
    RET_: maybe_spill;   if (v != small) { free(v); }        return rhs;
}

struct iv2d_region* iv2d_ret(builder *b, int ret) {
    push(b, (struct inst){RET, .rhs=ret});

    struct { int last_use, stack_slot; } *value = calloc((size_t)b->insts, sizeof *value);

    for (int i = 2; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;
        value[binst->lhs].last_use =
        value[binst->rhs].last_use = i;
    }

    struct program *p = malloc(sizeof *p + ((size_t)b->insts - 2) * sizeof *p->inst);
    *p = (struct program){.region={run_program}};
    value[0].stack_slot = p->stack_slots++;
    value[1].stack_slot = p->stack_slots++;

    struct inst* inst = p->inst;
    _Bool rhs_in_reg = 0;
    for (int i = 2; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;

        *inst++ = (struct inst){
            .op  = binst->op,
            .lhs =                   value[binst->lhs].stack_slot,
            .rhs = rhs_in_reg ? -1 : value[binst->rhs].stack_slot,
            .imm = binst->imm,
            .uni = binst->uni,
        };

        rhs_in_reg = (value[i].last_use == i+1 && b->inst[i+1].lhs != i)
                  || binst->op == RET;
        if (!rhs_in_reg) {
            value[i].stack_slot = p->stack_slots++;
        }
    }

    free(value);
    free(b->inst);
    free(b);
    return &p->region;
}
