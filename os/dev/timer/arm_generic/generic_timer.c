#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <instructionset.h>
#include <irq.h>
#include <timer.h>

#include "generic_timer.h"

volatile uint64_t g_system_timer = 0;

static struct generic_timer_config_desc default_generic_timer_desc = {
    .irq_phys = IRQN_NON_SECURE_PHYS_TIMER,
    .irq_virt = IRQN_VIRT_TIMER,
    .irq_sphys = IRQN_SECURE_PHYS_TIMER,
};

static uint64_t timer_cntfrq;

#define TIMER_RELOAD            (TICKS_PER_MS * 1000)

void generic_timer_init(timer_t *timer)
{
    assert(timer);

    MRS(CNTFRQ, timer_cntfrq);
    kprintf("arm generic timer freq %lld Hz\n", timer_cntfrq);

    uint32_t cnt_tval;
    MSR(CNT_TVAL, TIMER_RELOAD);
    MRS(CNT_TVAL, cnt_tval);
    kprintf("cnt_tval = %ld\n", cnt_tval);

    uint32_t cnt_ctl;
    MSR(CNT_CTL, 1);
    MRS(CNT_CTL, cnt_ctl);
    kprintf("cnt_ctl = %p\n", cnt_ctl);

    timer->irq = default_generic_timer_desc.irq_virt;
    timer->clock_freq = timer_cntfrq;
}