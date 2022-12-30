#include <chinos/config.h>

#include <k_stddef.h>
#include <k_string.h>
#include <k_assert.h>
#include <task.h>
#include <scheduler.h>
#include <registerset.h>

#include "context.h"

void context_init(struct context *ctx, u8 type)
{
    k_memset(ctx, 0, sizeof(struct context));

    if (type == TASK_TYPE_USER)
        ctx->regs[PSTATE] = PSTATE_USER;
    else
        ctx->regs[PSTATE] = PSTATE_KERNEL;
}

void context_set_entry(struct context *ctx, unsigned long entry)
{
    ctx->regs[PC] = entry;
}

void context_set_exit(struct context *ctx, unsigned long exit)
{
    ctx->regs[LR] = (unsigned long)exit;
}

void context_set_retcode(struct context *ctx, unsigned long retcode)
{
    ctx->regs[X0] = retcode;
}

void context_set_params(struct context *ctx, unsigned long p0, unsigned long p1, unsigned long p2, unsigned long p3,
                                                   unsigned long p4, unsigned long p5, unsigned long p6, unsigned long p7)
{
    ctx->regs[X0] = p0;
    ctx->regs[X1] = p1;
    ctx->regs[X2] = p2;
    ctx->regs[X3] = p3;
    ctx->regs[X4] = p4;
    ctx->regs[X5] = p5;
    ctx->regs[X6] = p6;
    ctx->regs[X7] = p7;
}

void context_set_stack(struct context *ctx, void *stack, u32 size)
{
    ctx->regs[SP] = (unsigned long)stack + size;
}

void context_set_spsr(struct context *ctx, unsigned long spsr)
{
    ctx->regs[PSTATE] = spsr;
}

void restore_current_context(void)
{
    struct tcb *current = this_task();
    assert(current);

#if 1
    context_t *ctx = &current->context;
    kprintf("ctx = %p\n", ctx);
    int i = 0;
    while(i < REGS_COUNT)
    {
        kprintf("X[%d, %p] = %p\n", i, &ctx->regs[i], ctx->regs[i]);
        i++;
    }
#endif

    asm volatile
    (
        // set sp_el1
        "mov     sp,  %0    \n"

        // restore PC -> ELR_EL1
        "ldr     x9, [sp, %1]   \n"
#ifdef CONFIG_HYPERVISOR_SUPPORT
        "msr     elr_el2, x9    \n"
#else
        "msr     elr_el1, x9    \n"
#endif
        // restore SP -> SP_EL0, PSTATE -> SPSR_EL1
        "ldp     x10, x11, [sp, %2]     \n"
        "msr     sp_el0,   x10          \n"
#ifdef CONFIG_HYPERVISOR_SUPPORT
        "msr     spsr_el2, x11          \n"
#else
        "msr     spsr_el1, x11          \n"
#endif        

        "ldp     x0,  x1,  [sp, #16 * 0]         \n"
        "ldp     x2,  x3,  [sp, #16 * 1]         \n"
        "ldp     x4,  x5,  [sp, #16 * 2]         \n"
        "ldp     x6,  x7,  [sp, #16 * 3]         \n"
        "ldp     x8,  x9,  [sp, #16 * 4]         \n"
        "ldp     x10, x11, [sp, #16 * 5]         \n"
        "ldp     x12, x13, [sp, #16 * 6]         \n"
        "ldp     x14, x15, [sp, #16 * 7]         \n"
        "ldp     x16, x17, [sp, #16 * 8]         \n"
        "ldp     x18, x19, [sp, #16 * 9]         \n"
        "ldp     x20, x21, [sp, #16 * 10]        \n"
        "ldp     x22, x23, [sp, #16 * 11]        \n"
        "ldp     x24, x25, [sp, #16 * 12]        \n"
        "ldp     x26, x27, [sp, #16 * 13]        \n"
        "ldp     x28, x29, [sp, #16 * 14]        \n"
        // restore lr(x30)
        "ldr     lr, [sp, #16 * 15]  \n"
        // return from exception
        "eret    \n"
        :
        : "r"(current->context.regs), "i"(CTX_OFFS_PC), "i"(CTX_OFFS_SP)
        : "memory"
    );
}

