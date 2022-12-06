#ifndef __CONTEXT_H
#define __CONTEXT_H

#include <k_types.h>

enum {
    X0 = 0,
    X1,
    X2,
    X3,
    X4,
    X5,
    X6,
    X7,
    X8,
    X9,
    X10,
    X11,
    X12,
    X13,
    X14,
    X15,
    X16,
    X17,
    X18,
    X19,
    X20,
    X21,
    X22,
    X23,
    X24,
    X25,
    X26,
    X27,
    X28,
    X29, FP = X29,
    X30, LR = X30,
    PC,
    SP,
    PSTATE,
    REGS_COUNT,
};

typedef struct context
{
    unsigned long regs[REGS_COUNT];
} context_t;

void context_init(struct context *ctx, u8 type);
void context_set_entry(struct context *ctx, unsigned long entry);
void context_set_exit(struct context *ctx, unsigned long exit);
void context_set_retcode(struct context *ctx, unsigned long retcode);
void context_set_params(struct context *ctx, unsigned long p0, unsigned long p1, unsigned long p2, unsigned long p3,
                                                   unsigned long p4, unsigned long p5, unsigned long p6, unsigned long p7);
void context_set_stack(struct context *ctx, void *stack, u32 size);
void restore_current_context(void);
#endif
