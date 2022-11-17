#include <chinos/config.h>

#include <k_stdint.h>
#include <k_string.h>
#include <task.h>
#include <scheduler.h>
#include <registerset.h>

#include "context.h"

void context_init(struct context *ctx, uint8_t type)
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

void context_set_reg_param(struct context *ctx, unsigned long p1, unsigned long p2, unsigned long p3)
{
    ctx->regs[X1] = p1;
    ctx->regs[X2] = p2;
    ctx->regs[X3] = p3;
}

void context_set_stack(struct context *ctx, void *stack, uint32_t size)
{
    ctx->regs[SP] = (unsigned long)stack + size;
}

void context_stack_init(struct context *ctx)
{
    struct context *_ctx = (struct context *)(ctx->regs[SP] - CTX_SIZE);
    k_memcpy(_ctx, ctx, sizeof(struct context));
}

//#define DEBUG_DUMP_CONTEXT
#ifdef DEBUG_DUMP_CONTEXT
static void context_dump(context_t *ctx)
{
    int i = 0;
    while(i < REGS_COUNT)
    {
        kprintf("R[%d] = 0x%x\n", i, ctx->regs[i]);
        i++;
    }
}
#endif

void restore_current_context(void)
{
    struct tcb *task = this_task();
    if (!task) {
        return ;
    }

    unsigned long *regs = task->context.regs;

#if 0
    // dump stack
    int j = 0;
    unsigned long *stack = (unsigned long *)(0xffffff800041c400-128*8);
    while(j < 128)
    {
        kprintf("X[%d, %p] = %p\n", j, stack, *stack);
        stack++;
        j++;
    }
#endif

#if 0
    context_t *ctx = (context_t *)(regs[SP] - CTX_SIZE);
    kprintf("ctx = %p, regs[SP] = %p\n", ctx, regs[SP]);
    int i = 0;
    while(i < REGS_COUNT)
    {
        kprintf("X[%d, %p] = %p\n", i, &ctx->regs[i], ctx->regs[i]);
        i++;
    }
#endif
    asm volatile
    (
        "mov     sp,  %0    \n"

        // restore PC -> ELR_EL1
        "ldr     x9, [sp, %2]   \n"
        "msr     elr_el1, x9    \n"

        // restore SP -> SP_EL0, PSTATE -> SPSR_EL1
        "ldp     x10, x11, [sp, %3]     \n"
        //"msr     sp_el0,   x10          \n"
        "msr     spsr_el1, x11          \n"

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
        "ldr     x30, [sp, %1]  \n"
        "add     sp, sp, %4\n"
        "eret    \n"
        :
        : "r"(regs[SP] - CTX_SIZE), "i"(CTX_OFFS_LR), "i"(CTX_OFFS_PC), "i"(CTX_OFFS_SP), "i"(CTX_SIZE)
        : "memory"
    );
}


