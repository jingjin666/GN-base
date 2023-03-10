#include <k_stdio.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <instructionset.h>
#include <irq.h>
#include <timer.h>

#include "generic_timer.h"

static struct generic_timer_config_desc default_generic_timer_desc = {
    .ns_el2_irq_phys = IRQN_NS_EL2_PHYS_TIMER,
    .ns_el2_irq_virt = IRQN_NS_EL2_VIRT_TIMER,

    .el1_irq_phys = IRQN_EL1_PHYS_TIMER,
    .el1_irq_virt = IRQN_EL1_VIRT_TIMER,

    .el3_irq_phys = IRQN_EL3_PHYS_TIMER
};

static uint64_t timer_cntfrq;

#define TIMER_RELOAD            (TICKS_PER_MS * 1000)

void generic_timer_init(timer_t *timer)
{
    assert(timer);

    // 注意CNTFRQ是系统定时器的主频，并不是CPU主频，QEMU默认的CPU主频为1GHz
    MRS(CNTFRQ, timer_cntfrq);
    kprintf("arm generic timer freq %lld Hz\n", timer_cntfrq);

#ifdef TIMER_DEADLINE
    uint64_t c_val;
    MRS(CNT_CT, c_val);
    kprintf("c_val = %ld\n", c_val);
    MSR(CNT_CVAL, c_val + TIMER_RELOAD);
#else
    uint32_t cnt_tval;
    MSR(CNT_TVAL, TIMER_RELOAD);
    MRS(CNT_TVAL, cnt_tval);
    kprintf("cnt_tval = %ld\n", cnt_tval);
#endif

    uint32_t cnt_ctl;
    MSR(CNT_CTL, 1);
    MRS(CNT_CTL, cnt_ctl);
    kprintf("cnt_ctl = %p\n", cnt_ctl);

#ifdef CONFIG_HYPERVISOR_SUPPORT
    timer->irq = default_generic_timer_desc.ns_el2_irq_phys;
#else
    timer->irq = default_generic_timer_desc.el1_irq_virt;
#endif
    timer->clock_freq = timer_cntfrq;
}
