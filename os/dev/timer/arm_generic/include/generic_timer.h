#ifndef __GENERIC_TIMER_H
#define __GENERIC_TIMER_H

#include <instructionset.h>
#include <const.h>
#include <board.h>

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

static inline void generic_timer_reset(u64 us)
{
    MSR(CNT_TVAL, us * TICKS_PER_US);
    MSR(CNT_CTL, 1);
}

static inline void generic_timer_clear(void)
{
    MSR(CNT_TVAL, 0);
    MSR(CNT_CTL, 0);
}

void timer_init(void);
#endif
