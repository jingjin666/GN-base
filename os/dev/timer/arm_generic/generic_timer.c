#include <k_stdio.h>
#include <k_stddef.h>
#include <uapi/types.h>
#include <instructionset.h>
#include <irq.h>

#include "generic_timer.h"

struct generic_timer_config_desc
{
    // Non-Secure Physicl Timer IRQ
    u16 irq_phys;
    // Virtual Timer IRQ
    u16 irq_virt;
    // Secure Physicl Timer IRQ
    u16 irq_sphys;
};

struct generic_timer_config_desc default_generic_timer_desc = {
    .irq_phys = IRQN_NON_SECURE_PHYS_TIMER,
    .irq_virt = IRQN_VIRT_TIMER,
    .irq_sphys = IRQN_SECURE_PHYS_TIMER,
};

static u64 timer_cntfrq;

#define TIMER_RELOAD            (TICKS_PER_MS * 1000)
static void reset_timer(void)
{
    u32 cnt_tval;
    MSR(CNT_TVAL, TIMER_RELOAD);
    MRS(CNT_TVAL, cnt_tval);
    kprintf("cnt_tval = %ld\n", cnt_tval);

    u32 cnt_ctl;
    MSR(CNT_CTL, 1);
    MRS(CNT_CTL, cnt_ctl);
    kprintf("cnt_ctl = %p\n", cnt_ctl);
}

static int timer_handler(int irq, void *context, void *arg)
{
    static int timer_irq_cnts = 0;
    kprintf("timer_handler %d\n", timer_irq_cnts++);
    reset_timer();
    return 0;
}

void timer_init(void)
{
    MRS(CNTFRQ, timer_cntfrq);
    kprintf("arm generic timer freq %lld Hz\n", timer_cntfrq);

    reset_timer();

    irq_attach(default_generic_timer_desc.irq_virt, timer_handler, NULL, NULL);
    up_enable_irq(default_generic_timer_desc.irq_virt);
}
