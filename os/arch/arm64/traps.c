#include <k_stdio.h>

#include <halt.h>
#include <instructionset.h>
#include <registerset.h>
#include <uapi/types.h>
#include <uapi/util.h>

void cel_sync_traps(void)
{
    u64 esr;

    kprintf("current el sync_traps\n");
    
    MRS("ESR_EL1", esr);
    kprintf("ESR_EL1 = %p\n", esr);
    
    u32 ec = esr >> ESR_EC_SHIFT;
    kprintf("ec = %p\n", ec);

    u64 far_el1;
    u64 afsr0_el1;

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

    halt();
}

void cel_serr_traps(void)
{
    kprintf("current el serr_traps\n");
}

void lel_sync_traps(void)
{
    u64 esr;

    kprintf("lower el sync_traps\n");
    
    MRS("ESR_EL1", esr);
    kprintf("ESR_EL1 = %p\n", esr);
    
    u32 ec = esr >> ESR_EC_SHIFT;
    kprintf("ec = %p\n", ec);

    u64 far_el1;
    u64 afsr0_el1;

    switch (ec)
    {
        case ESR_EC_LEL_DABT:
            kprintf("Data Abort from a lower Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("DFAR = %p, ADFSR = %p\n", far_el1, afsr0_el1);
            break;
        case ESR_EC_LEL_IABT:
            kprintf("Instruction Abort from a lower Exception level!\n");
            MRS("FAR_EL1", far_el1);
            MRS("AFSR0_EL1", afsr0_el1);
            kprintf("IFAR = %p, AIFSR = %p\n", far_el1, afsr0_el1);
            break;
        case ESR_EC_LEL_SVC64:
            kprintf("SVC instruction execution in AArch64 state!");
            break;
        case ESR_EC_LEL_HVC64:
            kprintf("HVC instruction execution in AArch64 state, when HVC is not disabled!\n");
            break;
        default:
            kprintf("unknown sync traps\n");
    }

    halt();
}

void lel_serr_traps(void)
{
    kprintf("lower el serr_traps\n");
}
