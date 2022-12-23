#ifndef __GENERIC_TIMER_H
#define __GENERIC_TIMER_H

#include <chinos/config.h>

#include <k_types.h>
#include <instructionset.h>
#include <const.h>
#include <board.h>
#include <timer.h>

#define CNTFRQ          "cntfrq_el0"

#define CNT_PCT         "cntpct_el0"
#define CNT_VCT         "cntvct_el0"

#ifdef CONFIG_HYPERVISOR_SUPPORT
#define CNTV_HYP_CTL        "cnthv_ctl_el2"
#define CNTP_HYP_CTL        "cnthp_ctl_el2"

#define CNTP_HYP_TVAL       "cnthp_tval_el2"
#define CNTP_HYP_CVAL       "cnthp_cval_el2"

#define CNTV_HYP_TVAL       "cnthv_tval_el2"
#define CNTV_HYP_CVAL       "cnthv_cval_el2"
#else
#define CNTV_CTL        "cntv_ctl_el0"
#define CNTP_CTL        "cntp_ctl_el0"

#define CNTP_TVAL       "cntp_tval_el0"
#define CNTP_CVAL       "cntp_cval_el0"

#define CNTV_TVAL       "cntv_tval_el0"
#define CNTV_CVAL       "cntv_cval_el0"
#endif

#ifdef CONFIG_HYPERVISOR_SUPPORT
#define CNT_CTL         CNTP_HYP_CTL
#define CNT_TVAL        CNTP_HYP_TVAL
#define CNT_CVAL        CNTP_HYP_CVAL
#define CNT_CT          CNT_PCT
#else
#define CNT_CTL         CNTV_CTL
#define CNT_TVAL        CNTV_TVAL
#define CNT_CVAL        CNTV_CVAL
#define CNT_CT          CNT_VCT
#endif

#define TICKS_PER_NS            (TIMER_CLOCK_HZ / ULL(1000) / ULL(1000) / ULL(1000))
#define TICKS_PER_US            (TIMER_CLOCK_HZ / ULL(1000) / ULL(1000))
#define TICKS_PER_MS            (TIMER_CLOCK_HZ / ULL(1000))
#define TICKS_PER_S             (TIMER_CLOCK_HZ)

struct generic_timer_config_desc
{
    // Non-Secure EL2 Physicl Timer IRQ
    uint16_t ns_el2_irq_phys;
    // Non-Secure EL2 Virtual Timer IRQ
    uint16_t ns_el2_irq_virt;

    // EL1 Physicl Timer IRQ
    uint16_t el1_irq_phys;
    // EL1 Virt Timer IRQ
    uint16_t el1_irq_virt;

    // EL3 Physicl Timer IRQ
    uint16_t el3_irq_phys;
};

static inline ticks_t get_current_time(void)
{
    ticks_t time;
    MRS(CNT_CT, time);
    return time;
}

static inline void set_deadline(ticks_t deadline)
{
    MSR(CNT_CVAL, deadline);
}

static inline void generic_timer_reset(uint64_t ms)
{
    MSR(CNT_TVAL, ms *TICKS_PER_MS);
    MSR(CNT_CTL, 1);
}

static inline void generic_timer_clear(void)
{
    MSR(CNT_TVAL, 0);
    MSR(CNT_CTL, 0);
}

void generic_timer_init(timer_t *timer);

#endif
