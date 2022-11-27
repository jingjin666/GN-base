#ifndef __GENERIC_TIMER_H
#define __GENERIC_TIMER_H

#include <k_stdint.h>
#include <instructionset.h>
#include <const.h>
#include <board.h>
#include <timer.h>

#define CNT_PCT     "cntpct_el0"
#define CNT_VCT     "cntvct_el0"

#define CNTV_CTL    "cntv_ctl_el0"
#define CNTP_CTL    "cntp_ctl_el0"

#define CNT_TVAL    "cntv_tval_el0"
#define CNT_CVAL    "cntv_cval_el0"

#define CNTFRQ      "cntfrq_el0"

#define CNT_CTL     CNTV_CTL

#define TICKS_PER_US            (TIMER_CLOCK_HZ / ULL(1000) / ULL(1000))
#define TICKS_PER_MS            (TIMER_CLOCK_HZ / ULL(1000))
#define TICKS_PER_S             (TIMER_CLOCK_HZ)

struct generic_timer_config_desc
{
    // Non-Secure Physicl Timer IRQ
    uint16_t irq_phys;
    // Virtual Timer IRQ
    uint16_t irq_virt;
    // Secure Physicl Timer IRQ
    uint16_t irq_sphys;
};

static inline void generic_timer_reset(uint64_t us)
{
    MSR(CNT_TVAL, us * TICKS_PER_US);
    MSR(CNT_CTL, 1);
}

static inline void generic_timer_clear(void)
{
    MSR(CNT_TVAL, 0);
    MSR(CNT_CTL, 0);
}

void generic_timer_init(timer_t *timer);

#endif
