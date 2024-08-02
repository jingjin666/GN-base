#include <chinos/config.h>

#include <k_stdio.h>
#include <k_assert.h>
#include <instructionset.h>
#include <scheduler.h>
#include <syscalls.h>
#include <fault.h>
#include <uapi/errors.h>
#include <esr.h>
#include <elf_loader.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT
#define ESR_ELx "ESR_EL2"
#define FAR_ELx "FAR_EL2"
#define AFSR0_ELx "AFSR0_EL2"
#else
#define ESR_ELx "ESR_EL1"
#define FAR_ELx "FAR_EL1"
#define AFSR0_ELx "AFSR0_EL1"
#endif

static const char *esr_class_str[] = {
	[0 ... ESR_ELx_EC_MAX]		= "UNRECOGNIZED EC",
	[ESR_ELx_EC_UNKNOWN]		= "Unknown/Uncategorized",
	[ESR_ELx_EC_WFx]		= "WFI/WFE",
	[ESR_ELx_EC_CP15_32]		= "CP15 MCR/MRC",
	[ESR_ELx_EC_CP15_64]		= "CP15 MCRR/MRRC",
	[ESR_ELx_EC_CP14_MR]		= "CP14 MCR/MRC",
	[ESR_ELx_EC_CP14_LS]		= "CP14 LDC/STC",
	[ESR_ELx_EC_FP_ASIMD]		= "ASIMD",
	[ESR_ELx_EC_CP10_ID]		= "CP10 MRC/VMRS",
	[ESR_ELx_EC_PAC]		= "PAC",
	[ESR_ELx_EC_CP14_64]		= "CP14 MCRR/MRRC",
	[ESR_ELx_EC_BTI]		= "BTI",
	[ESR_ELx_EC_ILL]		= "PSTATE.IL",
	[ESR_ELx_EC_SVC32]		= "SVC (AArch32)",
	[ESR_ELx_EC_HVC32]		= "HVC (AArch32)",
	[ESR_ELx_EC_SMC32]		= "SMC (AArch32)",
	[ESR_ELx_EC_SVC64]		= "SVC (AArch64)",
	[ESR_ELx_EC_HVC64]		= "HVC (AArch64)",
	[ESR_ELx_EC_SMC64]		= "SMC (AArch64)",
	[ESR_ELx_EC_SYS64]		= "MSR/MRS (AArch64)",
	[ESR_ELx_EC_SVE]		= "SVE",
	[ESR_ELx_EC_ERET]		= "ERET/ERETAA/ERETAB",
	[ESR_ELx_EC_FPAC]		= "FPAC",
	[ESR_ELx_EC_IMP_DEF]		= "EL3 IMP DEF",
	[ESR_ELx_EC_IABT_LOW]		= "IABT (lower EL)",
	[ESR_ELx_EC_IABT_CUR]		= "IABT (current EL)",
	[ESR_ELx_EC_PC_ALIGN]		= "PC Alignment",
	[ESR_ELx_EC_DABT_LOW]		= "DABT (lower EL)",
	[ESR_ELx_EC_DABT_CUR]		= "DABT (current EL)",
	[ESR_ELx_EC_SP_ALIGN]		= "SP Alignment",
	[ESR_ELx_EC_FP_EXC32]		= "FP (AArch32)",
	[ESR_ELx_EC_FP_EXC64]		= "FP (AArch64)",
	[ESR_ELx_EC_SERROR]		= "SError",
	[ESR_ELx_EC_BREAKPT_LOW]	= "Breakpoint (lower EL)",
	[ESR_ELx_EC_BREAKPT_CUR]	= "Breakpoint (current EL)",
	[ESR_ELx_EC_SOFTSTP_LOW]	= "Software Step (lower EL)",
	[ESR_ELx_EC_SOFTSTP_CUR]	= "Software Step (current EL)",
	[ESR_ELx_EC_WATCHPT_LOW]	= "Watchpoint (lower EL)",
	[ESR_ELx_EC_WATCHPT_CUR]	= "Watchpoint (current EL)",
	[ESR_ELx_EC_BKPT32]		= "BKPT (AArch32)",
	[ESR_ELx_EC_VECTOR32]		= "Vector catch (AArch32)",
	[ESR_ELx_EC_BRK64]		= "BRK (AArch64)",
};

const char *esr_get_class_string(u32 esr)
{
	return esr_class_str[ESR_ELx_EC(esr)];
}

static void svc_handler(void)
{
    unsigned long ret = -ENOSYS;
    struct tcb *current = this_task();
    assert(current);

    context_t *ctx = &current->context;
    unsigned long sn = ctx->regs[X8];

    kprintf("svc_handler: sn = %d\n", sn);

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
    
    MRS(ESR_ELx, esr);
    kprintf("ESR_ELx = %p\n", esr);

    kprintf("TRAPS# %s\n", esr_get_class_string(esr));

    unsigned long ec = esr >> ESR_ELx_EC_SHIFT;
    kprintf("ec = %p\n", ec);

    unsigned long iss = esr & ESR_ELx_ISS_MASK;
    kprintf("iss = %p\n", iss);

    unsigned long far_elx;
    unsigned long afsr0_elx;

    switch (ec)
    {
        case ESR_ELx_EC_DABT_CUR:
            kprintf("Data Abort taken without a change in Exception level!\n");
            MRS(FAR_ELx, far_elx);
            MRS(AFSR0_ELx, afsr0_elx);
            kprintf("DFAR = %p, ADFSR = %p\n", far_elx, afsr0_elx);
            break;
        case ESR_ELx_EC_IABT_CUR:
            kprintf("Instruction Abort taken without a change in Exception level!\n");
            MRS(FAR_ELx, far_elx);
            MRS(AFSR0_ELx, afsr0_elx);
            kprintf("IFAR = %p, AIFSR = %p\n", far_elx, afsr0_elx);
            break;
        case ESR_ELx_EC_ILL:
            kprintf("Illegal Execution state!\n");
            break;
        default:
            kprintf("cel_sync_traps: unknown sync traps\n");
    }

    PANIC();
}

void cel_serr_traps(void)
{
    kprintf("current el serr_traps\n");
}

struct stackframe {
	unsigned long fp;
	unsigned long pc;
	unsigned long prev_fp;
};

static inline void start_backtrace(struct stackframe *frame,
				   unsigned long fp, unsigned long pc)
{
	frame->fp = fp;
	frame->pc = pc;
	frame->prev_fp = 0;
}

struct stack_info {
	unsigned long low;
	unsigned long high;
};

/*
 * AArch64 PCS assigns the frame pointer to x29.
 *
 * A simple function prologue looks like this:
 * 	sub	sp, sp, #0x10
 *   	stp	x29, x30, [sp]
 *	mov	x29, sp
 *
 * A simple function epilogue looks like this:
 *	mov	sp, x29
 *	ldp	x29, x30, [sp]
 *	add	sp, sp, #0x10
 */

/*
 * Unwind from one frame record (A) to the next frame record (B).
 *
 * We terminate early if the location of B indicates a malformed chain of frame
 * records (e.g. a cycle), determined based on the location and fp value of A
 * and the location (but not the fp value) of B.
 */

#define READ_ONCE_NOCHECK(a)           (*(volatile uint64_t *)&(a))

static inline int on_accessible_stack(const struct tcb *tsk,
				       unsigned long sp,
				       struct stack_info *info)
{
    unsigned long low = (unsigned long)tsk->mm.start_stack;
	unsigned long high = (unsigned long)tsk->mm.end_stack;

	if (!low)
		return -1;

	if (sp < low || sp >= high)
		return -1;

	if (info) {
		info->low = low;
		info->high = high;
	}

	return 0;
}

int unwind_frame(struct tcb *tsk, struct stackframe *frame)
{
	unsigned long fp = frame->fp;
	struct stack_info info;

	if (fp & 0xf)
		return -EINVAL;

	if (on_accessible_stack(tsk, fp, &info) != 0)
		return -EINVAL;

	/*
	 * Record this frame record's values and location. The prev_fp and
	 * prev_type are only meaningful to the next unwind_frame() invocation.
	 */
	frame->fp = READ_ONCE_NOCHECK(*(unsigned long *)(fp));
	frame->pc = READ_ONCE_NOCHECK(*(unsigned long *)(fp + 8));

	frame->prev_fp = fp;

	/*
	 * Frames created upon entry from EL0 have NULL FP and PC values, so
	 * don't bother reporting these. Frames created by __noreturn functions
	 * might have a valid FP even if PC is bogus, so only terminate where
	 * both are NULL.
	 */
	if (!frame->fp && !frame->pc)
		return -EINVAL;

	return 0;
}

void dump_backtrace(void)
{
    struct stackframe frame;

    struct tcb *current = this_task();

    kprintf("%s# %s TID:%d TGID:%d\n", __FUNCTION__, current->name), current->tgid, current->tid;
    
    u64 pc = current->context.regs[PC];
    u64 sp = current->context.regs[SP];
    u64 fp = current->context.regs[FP];
    u64 lr = current->context.regs[LR];
    kprintf("PC %p SP %p FP %p LR %p\n", pc, sp, fp, lr);

    start_backtrace(&frame, fp, pc);

    kprintf("PC >>> %p :: %s\n", frame.pc, get_symbol_name_by_addr(pc, current->mm.elf));
	kprintf("PC >>> %p :: %s\n", lr, get_symbol_name_by_addr(lr, current->mm.elf));

    while (!unwind_frame(current, &frame)) {
        kprintf("PC >>> %p :: %s\n", frame.pc, get_symbol_name_by_addr(frame.pc, current->mm.elf));
    }
}

void lel_sync_traps(void)
{
    unsigned long esr;

    kprintf("lower el sync_traps\n");
    
    MRS(ESR_ELx, esr);
    kprintf("ESR_ELx = %p\n", esr);

    kprintf("TRAPS# %s\n", esr_get_class_string(esr));

    unsigned long ec = esr >> ESR_ELx_EC_SHIFT;
    kprintf("ec = %p\n", ec);

    unsigned long far_elx;
    unsigned long afsr0_elx;

    switch (ec)
    {
        case ESR_ELx_EC_WFx:
#ifdef CONFIG_HYPERVISOR_SUPPORT
            vcpu_store();
#endif
            schedule();

#ifdef CONFIG_HYPERVISOR_SUPPORT
            vcpu_restore();
#endif
            restore_current_context();
            break;
        case ESR_ELx_EC_DABT_LOW:
            kprintf("Data Abort from a lower Exception level!\n");
            dump_backtrace();

            MRS(FAR_ELx, far_elx);
            MRS(AFSR0_ELx, afsr0_elx);
            kprintf("DFAR = %p, ADFSR = %p\n", far_elx, afsr0_elx);
            do_page_fault(far_elx);
            break;
        case ESR_ELx_EC_IABT_LOW:
            kprintf("Instruction Abort from a lower Exception level!\n");
            MRS(FAR_ELx, far_elx);
            MRS(AFSR0_ELx, afsr0_elx);
            kprintf("IFAR = %p, AIFSR = %p\n", far_elx, afsr0_elx);
            break;
        case ESR_ELx_EC_SVC64:
            //kprintf("SVC instruction execution in AArch64 state!\n");
            svc_handler();
            break;
        case ESR_ELx_EC_HVC64:
            kprintf("HVC instruction execution in AArch64 state, when HVC is not disabled!\n");
            break;
        case ESR_ELx_EC_ILL:
            kprintf("Illegal Execution state!\n");
            break;
        default:
            kprintf("lel_sync_traps: unknown sync traps\n");
    }

    PANIC();
}

void lel_serr_traps(void)
{
    kprintf("lower el serr_traps\n");
}
