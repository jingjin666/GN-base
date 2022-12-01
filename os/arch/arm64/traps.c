#include <k_stdio.h>
#include <k_assert.h>
#include <instructionset.h>
#include <registerset.h>
#include <scheduler.h>
#include <syscalls.h>
#include <fault.h>
#include <uapi/errors.h>

static void svc_handler(void)
{
    unsigned long ret = -ENOSYS;
    struct tcb *current = this_task();
    assert(current);

    context_t *ctx = &current->context;
    unsigned long sn = ctx->regs[X8];

    //kprintf("svc_handler: sn = %d\n", sn);

    if (sn < ARRAY_SIZE(svc_handlers)) {
        if (svc_handlers[sn])
            ret = svc_handlers[sn](ctx->regs);
        else
            ret = -ENOSYS;
    } else {
        ret = -ENOSYS;
        kprintf("Unsupported svc sn = %d\n", sn);
    }

    context_set_retcode(ctx, ret);

    schedule();

    restore_current_context();
}

void cel_sync_traps(void)
{
    unsigned long esr;

    kprintf("current el sync_traps\n");
    
    MRS("ESR_EL1", esr);
    kprintf("ESR_EL1 = %p\n", esr);
    
    unsigned long ec = esr >> ESR_EC_SHIFT;
    kprintf("ec = %p\n", ec);

    unsigned long far_el1;
    unsigned long afsr0_el1;

    switch (ec)
    {
        case ESR_EC_CEL_DABT:
            kprintf("Data Abort taken without a change in Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("DFAR = %p, ADFSR = %p\n", far_el1, afsr0_el1);
            break;
        case ESR_EC_CEL_IABT:
            kprintf("Instruction Abort taken without a change in Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("IFAR = %p, AIFSR = %p\n", far_el1, afsr0_el1);
            break;
        default:
            kprintf("unknown sync traps\n");
    }

    PANIC();
}

void cel_serr_traps(void)
{
    kprintf("current el serr_traps\n");
}

void lel_sync_traps(void)
{
    unsigned long esr;

    //kprintf("lower el sync_traps\n");
    
    MRS("ESR_EL1", esr);
    //kprintf("ESR_EL1 = %p\n", esr);
    
    unsigned long ec = esr >> ESR_EC_SHIFT;
    //kprintf("ec = %p\n", ec);

    unsigned long far_el1;
    unsigned long afsr0_el1;

    switch (ec)
    {
        case ESR_EC_LEL_DABT:
            kprintf("Data Abort from a lower Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("DFAR = %p, ADFSR = %p\n", far_el1, afsr0_el1);
            do_page_fault(far_el1);
            break;
        case ESR_EC_LEL_IABT:
            kprintf("Instruction Abort from a lower Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("IFAR = %p, AIFSR = %p\n", far_el1, afsr0_el1);
            break;
        case ESR_EC_LEL_SVC64:
            //kprintf("SVC instruction execution in AArch64 state!\n");
            svc_handler();
            break;
        case ESR_EC_LEL_HVC64:
            kprintf("HVC instruction execution in AArch64 state, when HVC is not disabled!\n");
            break;
        default:
            kprintf("unknown sync traps\n");
    }

    PANIC();
}

void lel_serr_traps(void)
{
    kprintf("lower el serr_traps\n");
}
