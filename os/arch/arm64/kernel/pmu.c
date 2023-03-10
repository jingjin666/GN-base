#include <chinos/config.h>

#ifdef CONFIG_ARM_PMU

#include <k_stdio.h>
#include <k_assert.h>
#include <uapi/util.h>
#include <instructionset.h>
#include <sysreg.h>
#include <board.h>
#include <gic.h>

#include "pmu.h"

#define PMU_ARMV8A_NUM_COUNTERS 4

#define PMU_ARMV8A_COUNTER_CCNT 31


#define PMCR        "PMCR_EL0"          /* Performance Monitors Control Register */

#define PMUSERENR   "PMUSERENR_EL0"     /* Performance Monitors User Enable Register */

#define PMCNTENCLR  "PMCNTENCLR_EL0"    /* Performance Monitors Count Enable Clear register */
#define PMCNTENSET  "PMCNTENSET_EL0"    /* Performance Monitors Count Enable Set register */

#define PMOVSCLR    "PMOVSCLR_EL0"      /* Performance Monitors Overflow Flag Status Clear Register */
#define PMOVSSET    "PMOVSSET_EL0"      /* Performance Monitors Overflow Flag Status Set Register */

#define PMINTENCLR  "PMINTENCLR_EL1"    /* Performance Monitors Interrupt Enable Clear register */
#define PMINTENSET  "PMINTENSET_EL1"    /* Performance Monitors Interrupt Enable set register */

#define PMSELR      "PMSELR_EL0"        /* Performance Monitors Event Counter Selection Register */
#define PMXEVCNTR   "PMXEVCNTR_EL0"     /* Performance Monitors Selected Event Count Register */
#define PMXEVTYPER  "PMXEVTYPER_EL0"    /* Performance Monitors Selected Event Type Register */

#define PMCCNTR     "PMCCNTR_EL0"       /* Performance Monitors Cycle Count Register */

#define EVT_NUM 0
static u64 GENERIC_EVENTS[] = {
    ARMV8_PMUV3_PERFCTR_CPU_CYCLES,
    ARMV8_PMUV3_PERFCTR_L1I_CACHE_REFILL,
    ARMV8_PMUV3_PERFCTR_L1D_CACHE_REFILL,
    ARMV8_PMUV3_PERFCTR_L1I_TLB_REFILL,
    ARMV8_PMUV3_PERFCTR_L1D_TLB_REFILL,
    ARMV8_PMUV3_PERFCTR_INST_RETIRED,
    ARMV8_PMUV3_PERFCTR_BR_MIS_PRED,
    ARMV8_PMUV3_PERFCTR_MEM_ACCESS,
};

static char *GENERIC_EVENT_NAMES[] = {
    "CPU cycles",
    "L1 i-cache misses",
    "L1 d-cache misses",
    "L1 i-tlb misses",
    "L1 d-tlb misses",
    "Instructions",
    "Branch mispredictions",
    "Memory accesses",
};

/* Number of generic counters */
#define PMU_NUM_GENERIC_EVENTS ARRAY_SIZE(GENERIC_EVENTS)

static void pmu_write_ccfiltr(u64 ccfiltr)
{
    write_sysreg(ccfiltr, PMCCFILTR_EL0);
}

static u64 pmu_read_ccfiltr(void)
{
    u64 pmu_ccfiltr = read_sysreg(PMCCFILTR_EL0);
    return pmu_ccfiltr;
}

static void pmu_write_cr(u64 cr)
{
    write_sysreg(cr, PMCR_EL0);
}

static u64 pmu_read_cr(void)
{
    u64 pmucr = read_sysreg(PMCR_EL0);
    return pmucr;
}

static u64 pmu_read_ceid0(void)
{
    u64 ceid0 = read_sysreg(PMCEID0_EL0);
    return ceid0;
}

static u64 pmu_read_ceid1(void)
{
    u64 ceid1 = read_sysreg(PMCEID1_EL0);
    return ceid1;
}

static void pmu_write_inten_set(u64 mask)
{
    write_sysreg(mask, PMINTENSET_EL1);
}

static u64 pmu_read_inten_set(void)
{
    u64 inten_set = read_sysreg(PMINTENSET_EL1);
    return inten_set;
}

static void pmu_write_inten_clear(u64 mask)
{
    write_sysreg(mask, PMINTENCLR_EL1);
}

static u64 pmu_read_inten_clear(void)
{
    u64 inten_clear = read_sysreg(PMINTENCLR_EL1);
    return inten_clear;
}

static void pmu_write_ovs_set(u64 mask)
{
    write_sysreg(mask, PMOVSSET_EL0);
}

static u64 pmu_read_ovs_set(void)
{
    u64 ovs_set = read_sysreg(PMOVSSET_EL0);
    return ovs_set;
}

static void pmu_write_ovs_clear(u64 mask)
{
    write_sysreg(mask, PMOVSCLR_EL0);
}

static u64 pmu_read_ovs_clear(void)
{
    u64 ovs_clear = read_sysreg(PMOVSCLR_EL0);
    return ovs_clear;
}

static void pmu_write_useren(u64 useren)
{
    write_sysreg(useren, PMUSERENR_EL0);
}

static u64 pmu_read_useren(void)
{
    u64 useren = read_sysreg(PMUSERENR_EL0);
    return useren;
}

static void pmu_write_ccnt(u64 ccnt)
{
    write_sysreg(ccnt, PMCCNTR_EL0);
}

static u64 pmu_read_ccnt(void)
{
    u64 ccnt = read_sysreg(PMCCNTR_EL0);
    return ccnt;
}

static void pmu_write_cnten_set(u32 mask)
{
    write_sysreg(mask, PMCNTENSET_EL0);
}

static u64 pmu_read_cnten_set(void)
{
    u32 pmu_cnt_enset = read_sysreg(PMCNTENSET_EL0);
    return pmu_cnt_enset;
}

static void pmu_write_cnten_clear(u32 mask)
{
    write_sysreg(mask, PMCNTENCLR_EL0);
}

static u32 pmu_read_cnten_clear(void)
{
    u32 pmu_cnt_enclr = read_sysreg(PMCNTENCLR_EL0);
    return pmu_cnt_enclr;
}

static void pmu_write_sel(u32 sel)
{
    assert(sel >= 0 && sel <= 31);

    write_sysreg(sel, PMSELR_EL0);
    //u64 pmu_event_select = read_sysreg(PMSELR_EL0);
    //kprintf("pmu_select = %p\n", pmu_event_select);
}

static void pmu_write_type(u32 type)
{
#ifdef CONFIG_HYPERVISOR_SUPPORT
    write_sysreg(type | ARMV8_PMU_INCLUDE_EL2, PMXEVTYPER_EL0);
#else
    write_sysreg(type, PMXEVTYPER_EL0);
#endif    
    /* 配置好PMSELR_EL0后，访问PMXEVTYPER_EL0，等同于访问PMEVTYPER<n>_EL0 */
    //u64 pmu_event_type = read_sysreg(PMXEVTYPER_EL0);
    //kprintf("pmu_event_type = %p\n", pmu_event_type);
}

static void pmu_write_Xevtcnt(u32 cnt)
{
    write_sysreg(cnt, PMXEVCNTR_EL0);

    /* 配置好PMSELR_EL0后，访问PMXEVCNTR_EL0，等同于访问PMEVCNTR<n>_EL0 */
    //u64 pmu_event_cnt = read_sysreg(PMXEVCNTR_EL0);
    //kprintf("pmu_event_cnt = %p\n", pmu_event_cnt);
}

static u64 pmu_read_Xevtcnt(void)
{
    /* 配置好PMSELR_EL0后，访问PMXEVCNTR_EL0，等同于访问PMEVCNTR<n>_EL0 */
    u64 pmu_event_cnt = read_sysreg(PMXEVCNTR_EL0);
    //kprintf("pmu_event_cnt = %p\n", pmu_event_cnt);
    return pmu_event_cnt;
}

static void pmu_set_count_event(u64 evt_sel, u64 evt_id)
{
    pmu_write_sel(evt_sel);
    pmu_write_type(evt_id);
    pmu_write_Xevtcnt(0);
}

static u64 pmu_get_count_event(u64 evt_sel)
{
    pmu_write_sel(evt_sel);
    return pmu_read_Xevtcnt();
}


/*******************************************************************/

static void bench_start_counters(u64 mask)
{
    pmu_write_cnten_set(mask);
}

static void bench_stop_counters(u64 mask)
{
    pmu_write_cnten_clear(mask & ~bit(PMU_ARMV8A_COUNTER_CCNT));
}

static void bench_reset_counters(void)
{
    u64 pmu_cr = pmu_read_cr();
    pmu_cr |= ARMV8_PMU_PMCR_P;
    pmu_write_cr(pmu_cr);
}

u64 bench_get_counter(u64 evt_counter)
{
    // store cnten config
    //u64 set = pmu_read_cnten_set();

    // stop counter will clear Xevcntr
    //pmu_write_cnten_clear(bit(evt_counter));

    u64 evt_cnt = pmu_get_count_event(evt_counter);

    // restore cnten config
    //pmu_write_cnten_set(set);
    
    return evt_cnt;
}

/*
 * @param：n_events        [ 自定义事件数 ]
 * @param：events          [ 自定义事件的事件id和type ]
 * @param：n_counters      [ 硬件支持的最大事件数 ]
 */
static u64 bench_register_generic_counters(u64 n_events, u64 *events, u64 n_counters)
{
    u64 mask = 0;

    for (u64 i = 0; i < n_counters; i++) {
        if (i >= n_events) {
            break;
        }
        pmu_set_count_event(i, events[i]);
        mask |= bit(i);
    }

    bench_reset_counters();
    bench_start_counters(mask);
    return mask;
}

static u64 bench_get_num_counters()
{
    return bitfield_get(pmu_read_cr(), 11, 5);
}

u64 bench_get_cycle_count(void)
{
    u64 cycle_count = pmu_read_ccnt();
    return cycle_count;
}

#include <scheduler.h>
int pmu_evt_handler(int irq, void *context, void *arg)
{
    //kprintf("---\npmu_evt_handler\n");

    //u64 cycle_count = bench_get_cycle_count();
    //kprintf("cycle_count = %lu\n", cycle_count);   

    //u64 bench_cnt = bench_get_counter(EVT_NUM);
    //kprintf("bench_cnt = %p\n", bench_cnt);

    u64 pmu_ovs_clr = pmu_read_ovs_clear();
    //kprintf("pmu_ovs_clr = %p\n", pmu_ovs_clr);

    u64 mask = pmu_ovs_clr | 0xffffffff;
    pmu_write_ovs_clear(mask);

    // stop event counter
    pmu_write_cr(pmu_read_cr() & ~ARMV8_PMU_PMCR_E);

    //bench_cnt = bench_get_counter(EVT_NUM);
    //kprintf("bench_cnt = %p\n", bench_cnt);

    // restart event counter
    pmu_write_cr(pmu_read_cr() | ARMV8_PMU_PMCR_E);

    pmu_write_sel(EVT_NUM);
    //pmu_write_Xevtcnt(0xc0000000);
    pmu_write_Xevtcnt(0x40000000);
    struct tcb *current = this_task();
    kprintf("LR = %p, PC = %p, PSTATE = %p\n", current->context.regs[LR], current->context.regs[PC], current->context.regs[PSTATE]);

    //u64 xevt_cnt = pmu_read_Xevtcnt();
    //kprintf("xevt_cnt = %p\n", xevt_cnt);

    //bench_cnt = bench_get_counter(EVT_NUM);
    //kprintf("bench_cnt = %lu\n", bench_cnt);

    //assert(0);
}

void pmu_get_suppurt_event_ids(void)
{
    // FEAT_PMUv3p1 enable : IDhi<n>, bit[n+32], for n = 31 to 0, event id = 0x4000+n
    // ID<n>, bit[n], for n = 31 to 0, event id = 0x0000+n
    u64 ceid0 = pmu_read_ceid0();
    kprintf("ceid0 = %p\n", ceid0);

    // FEAT_PMUv3p1 enable : IDhi<n>, bit[n+32], for n = 31 to 0, event id = 0x4020+n
    // ID<n>, bit[n], for n = 31 to 0, event id = 0x0020+n
    u64 ceid1 = pmu_read_ceid0();
    kprintf("ceid1 = %p\n", ceid1);
}

void pmu_init(void)
{
    kprintf("\n\n------pmu init------\n\n");

    // get cycle count filter
    u64 pmu_ccfiltr = pmu_read_ccfiltr();
    kprintf("pmu_ccfiltr = %p\n", pmu_ccfiltr);

    pmu_write_ccfiltr(pmu_ccfiltr | ARMV8_PMU_EXCLUDE_EL0);

#ifdef CONFIG_HYPERVISOR_SUPPORT
    pmu_write_ccfiltr(pmu_ccfiltr | ARMV8_PMU_INCLUDE_EL2);
#endif

    // get support event ids
    pmu_get_suppurt_event_ids();

    // disable all event
    pmu_write_cnten_clear(0xFFFFFFFF);

    u64 pmucr = pmu_read_cr();
    kprintf("pmucr = %p\n", pmucr);
    pmu_write_cr(pmucr & ~ARMV8_PMU_PMCR_D);
    pmu_write_cr(pmucr | ARMV8_PMU_PMCR_P | ARMV8_PMU_PMCR_C);
    pmu_write_cr(pmucr | ARMV8_PMU_PMCR_E);
    pmucr = pmu_read_cr();
    kprintf("pmucr = %p\n", pmucr);

    u64 pmu_user_en = pmu_read_useren();
    kprintf("pmu_user_en = %p\n", pmu_user_en);
    pmu_write_useren(bit(0));
    pmu_user_en = pmu_read_useren();
    kprintf("pmu_user_en = %p\n", pmu_user_en);

    // enable counter cnt
    pmu_write_cnten_set(bit(PMU_ARMV8A_COUNTER_CCNT));
    u64 pmu_cnt_enset = pmu_read_cnten_set();
    kprintf("pmu_cnt_enset = %p\n", pmu_cnt_enset);
    u64 pmu_cnt_enclr = pmu_read_cnten_clear();
    kprintf("pmu_cnt_enclr = %p\n", pmu_cnt_enclr);

#if 1
    // delay 1 second test
    u64 cpu_freq = CPU_FREQ / 3;
    u64 cycle_count_start = bench_get_cycle_count();
    while(cpu_freq--);
    u64 cycle_count_end = bench_get_cycle_count();

    s64 diff = cycle_count_end - cycle_count_start;
    float time = (float)diff / CPU_FREQ;
    kprintf("\ncycle_count_start = %lu, cycle_count_end = %lu, diff = %ld, use time = %.7f second\n", cycle_count_start, cycle_count_end, diff, time);
#endif

    kprintf("\n------------------------\n");
}

void pmu_enable_interrupt(void)
{
    u64 bench_cnt;

    irq_attach(IRQN_PMU, pmu_evt_handler, 0, 0);
    irq_enable(IRQN_PMU);

#if 1
    pmu_set_count_event(EVT_NUM, ARMV8_PMUV3_PERFCTR_CPU_CYCLES);
    bench_reset_counters();
    bench_start_counters(bit(EVT_NUM));
#else
    u64 performance_counter_supported = bench_get_num_counters();
    kprintf("performance_counter_supported = %d\n", performance_counter_supported);
    bench_register_generic_counters(PMU_NUM_GENERIC_EVENTS, GENERIC_EVENTS, performance_counter_supported);
#endif

#if 1
    kprintf("________________________\n");

    bench_cnt = bench_get_counter(EVT_NUM);
    kprintf("1.bench_cnt = %p\n", bench_cnt);

    int i,c=0;
    for(i=0;i<100000000;i++)
    {
        c+=i*i;
        c-=i*100;
        c+=i*i*i/100;
    }

    bench_cnt = bench_get_counter(EVT_NUM);
    kprintf("2.bench_cnt = %p\n", bench_cnt);

    kprintf("________________________\n");

    for(i=0;i<100000000;i++)
    {
        c+=i*i;
        c-=i*100;
        c+=i*i*i/100;
    }
#endif

    //u64 cycle_count = bench_get_cycle_count();
    //kprintf("cycle_count = %lu\n", cycle_count);

#if 1
    u64 pmu_interrupt_enset = pmu_read_inten_set();
    kprintf("pmu_interrupt_enset = %p\n", pmu_interrupt_enset);
    pmu_write_inten_set(bit(EVT_NUM));
    pmu_interrupt_enset = pmu_read_inten_set();
    kprintf("pmu_interrupt_enset = %p\n", pmu_interrupt_enset);

    u64 pmu_ovs_set = pmu_read_ovs_set();
    kprintf("pmu_ovs_set = %p\n", pmu_ovs_set);
#endif
    //cleanInvalidateCaches();
    //cleanInvalidateCaches();
    //cleanInvalidateCaches();
    //cleanInvalidateCaches();
    bench_cnt = bench_get_counter(EVT_NUM);
    kprintf("3.bench_cnt = %p\n", bench_cnt);
}
#endif
