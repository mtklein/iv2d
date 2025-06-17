#include "iv2d_vm.h"
#include "len.h"
#include <stdlib.h>
#include <string.h>

struct inst {
    enum { IMM,UNI,X,Y,RET,ABS,SQT,SQR,INV,ADD,SUB,MUL,MIN,MAX} op;
    int rhs;
    union {
        int          lhs;
        float        imm;
        float const *ptr;
    };
};

static _Bool uses_lhs(struct inst const *inst) { return inst->op >= ADD; }
static _Bool uses_rhs(struct inst const *inst) { return inst->op >= RET; }


struct hash {
    int vals,slots;
    struct slot { unsigned hash; int val; } *slot;
};

static void just_insert(struct hash *h, unsigned hash, int val) {
    unsigned const mask = (unsigned)h->slots - 1;
    unsigned          i = hash & mask;
    while (h->slot[i].hash) {
        i = (i+1) & mask;
    }
    h->slot[i].hash = hash;
    h->slot[i].val  = val;
}
static void insert(struct hash *h, unsigned hash, int val) {
    hash = hash ? hash : 1;
    if (h->vals/3 >= h->slots/4) {
        struct hash grown = {.vals=h->vals, .slots=h->slots ? 2*h->slots : 2};
        grown.slot = calloc((size_t)grown.slots, sizeof *grown.slot);
        for (int i = 0; i < h->slots; i++) {
            if (h->slot[i].hash) {
                just_insert(&grown, h->slot[i].hash, h->slot[i].val);
            }
        }
        free(h->slot);
        *h = grown;
    }
    just_insert(h, hash, val);
    h->vals++;
}
static _Bool lookup(struct hash h, unsigned hash, _Bool(*match)(int, void*), void *ctx) {
    if (h.slot) {
        hash = hash ? hash : 1;
        unsigned const mask = (unsigned)h.slots - 1;
        unsigned          i = hash & mask;
        while (h.slot[i].hash) {
            if (h.slot[i].hash == hash && match(h.slot[i].val, ctx)) {
                return 1;
            }
            i = (i+1) & mask;
        }
    }
    return 0;
}

typedef struct iv2d_builder {
    struct inst *inst;
    int          insts,padding;
    struct hash  dedup;
} builder;

struct dedup_ctx {
    builder     const *b;
    struct inst const *inst;
    int                id, padding;
};
static _Bool dedup(int id, void *ctx) {
    struct dedup_ctx *dedup = ctx;
    if (0 == memcmp(dedup->b->inst+id, dedup->inst, sizeof *dedup->inst)) {
        dedup->id = id;
        return 1;
    }
    return 0;
}
static unsigned fnv1a(void const *ptr, size_t len) {
    unsigned hash = 0x811c9dc5;
    for (unsigned char const *byte=ptr, *end=byte+len; byte != end; byte++) {
        hash ^= *byte;
        __builtin_mul_overflow(hash, 0x01000193, &hash);
    }
    return hash;
}
static _Bool is_pow2_or_zero(int n) {
    return 0 == (n & (n-1));
}
static int push(builder *b, struct inst inst) {
    unsigned const hash = fnv1a(&inst, sizeof inst);
    struct dedup_ctx dedup_ctx = {.b=b, .inst=&inst};
    if (lookup(b->dedup, hash, dedup, &dedup_ctx)) {
        return dedup_ctx.id;
    }

    if (is_pow2_or_zero(b->insts)) {
        b->inst = realloc(b->inst, (size_t)(b->insts ? 2*b->insts : 1) * sizeof *b->inst);
    }
    int const id = b->insts++;
    b->inst[id] = inst;
    insert(&b->dedup, hash, id);
    return id;
}

builder* iv2d_builder(void) {
    builder *b = calloc(1, sizeof *b);
    return b;
}

static void sort(int *l, int *r) { if (*r < *l) { int const t = *r; *r = *l; *l = t; } }

int iv2d_x(builder *b) { return push(b, (struct inst){.op=X}); }
int iv2d_y(builder *b) { return push(b, (struct inst){.op=Y}); }

int iv2d_imm(builder *b, float        imm) { return push(b, (struct inst){.op=IMM, .imm=imm}); }
int iv2d_uni(builder *b, float const *ptr) { return push(b, (struct inst){.op=UNI, .ptr=ptr}); }

int iv2d_abs   (builder *b, int v) { return push(b, (struct inst){.op=ABS, .rhs=v}); }
int iv2d_sqrt  (builder *b, int v) { return push(b, (struct inst){.op=SQT, .rhs=v}); }
int iv2d_square(builder *b, int v) { return push(b, (struct inst){.op=SQR, .rhs=v}); }
int iv2d_inv   (builder *b, int v) { return push(b, (struct inst){.op=INV, .rhs=v}); }

int iv2d_sub(builder *b, int l, int r) {
    return push(b, (struct inst){.op=SUB, .lhs=l, .rhs=r});
}
int iv2d_add(builder *b, int l, int r) {
    sort(&l,&r);
    return push(b, (struct inst){.op=ADD, .lhs=l, .rhs=r});
}
int iv2d_mul(builder *b, int l, int r) {
    sort(&l,&r);
    return push(b, (struct inst){.op=MUL, .lhs=l, .rhs=r});
}
int iv2d_min(builder *b, int l, int r) {
    sort(&l,&r);
    return push(b, (struct inst){.op=MIN, .lhs=l, .rhs=r});
}
int iv2d_max(builder *b, int l, int r) {
    sort(&l,&r);
    return push(b, (struct inst){.op=MAX, .lhs=l, .rhs=r});
}

int iv2d_mad(builder *b, int x, int y, int z) { return iv2d_add(b, iv2d_mul(b, x,y), z); }


struct program {
    struct iv2d_region region;
    int         vals, imms;
    struct inst inst[];
};

struct scratch {
    struct program const *active_program;
    void                 *padding;
    iv                    val[];
};

static iv run_program(struct iv2d_region const *region, iv x, iv y) {
    struct program const *p = (struct program const*)region;

    _Thread_local static struct scratch *scratch = NULL;
    if (scratch == NULL || scratch->active_program != p) {
        scratch = realloc(scratch, sizeof *scratch + (size_t)p->vals * sizeof *scratch->val);
        for (int i = 0; i < p->imms; i++) {
            scratch->val[i] = as_iv(p->inst[i].imm);
        }
        scratch->active_program = p;
    }

    static void* const dispatch[] = {
        NULL,
        &&uni,
        &&opx,
        &&opy,
        &&ret,
        &&abs,
        &&sqt,
        &&sqr,
        &&inv,
        &&add,
        &&sub,
        &&mul,
        &&min,
        &&max,
    };
    struct inst const *inst = p->inst + p->imms;
    iv *v = scratch->val, *next = v + p->imms;
    goto *dispatch[inst->op];

    uni: *next++ = as_iv(*inst->ptr);                   inst++; goto *dispatch[inst->op];
    opx: *next++ = x;                                   inst++; goto *dispatch[inst->op];
    opy: *next++ = y;                                   inst++; goto *dispatch[inst->op];
    abs: *next++ = iv_abs(              v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    sqt: *next++ = iv_sqrt(             v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    sqr: *next++ = iv_square(           v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    inv: *next++ = iv_inv(              v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    add: *next++ = iv_add(v[inst->lhs], v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    sub: *next++ = iv_sub(v[inst->lhs], v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    mul: *next++ = iv_mul(v[inst->lhs], v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    min: *next++ = iv_min(v[inst->lhs], v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    max: *next++ = iv_max(v[inst->lhs], v[inst->rhs]);  inst++; goto *dispatch[inst->op];
    ret: return v[inst->rhs];
}

struct iv2d_region const* iv2d_ret(builder *b, int ret) {
    push(b, (struct inst){.op=RET, .rhs=ret});

    int *new_index = calloc((size_t)b->insts, sizeof *new_index);

    struct program *p = malloc(sizeof *p + (size_t)b->insts * sizeof *p->inst);
    *p = (struct program){.region={run_program}, .vals=b->insts-1};

    int ix = 0;
    for (int imm = 2; imm --> 0;)
    for (int i = 0; i < b->insts; i++) {
        struct inst const *binst = b->inst+i;
        if (imm == (binst->op == IMM)) {
            p->imms += (binst->op == IMM);
            p->inst[ix] = *binst;
            if (uses_lhs(binst)) { p->inst[ix].lhs = new_index[binst->lhs]; }
            if (uses_rhs(binst)) { p->inst[ix].rhs = new_index[binst->rhs]; }
            new_index[i] = ix++;
        }
    }

    free(new_index);
    free(b->dedup.slot);
    free(b->inst);
    free(b);
    return &p->region;
}
