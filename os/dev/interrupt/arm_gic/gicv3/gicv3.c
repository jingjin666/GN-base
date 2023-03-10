#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stddef.h>
#include <k_stdbool.h>
#include <k_assert.h>
#include <uapi/util.h>
#include <barrier.h>
#include <instructionset.h>
#include <board.h>
#include <irq.h>
#include <task.h>
#include <scheduler.h>
#include <timer.h>
#include <sysreg.h>

#include <gic.h>

struct gicv3_config_desc
{
    uint64_t mmio_base;
    uint64_t gicd_offset;
    uint64_t gicr_offset;
    uint64_t gicr_stride;
    uint64_t reserved0;
    uint64_t ipi_base;
    uint64_t optional;
    uint64_t reserved1[3];
};

struct gicv3_config_desc defualt_gicv3_config_desc = 
{
    .mmio_base = GIC_VBASE,
    .gicd_offset = 0x00000,
    .gicr_offset = 0xa0000,
    .gicr_stride = 0x20000,
    .ipi_base = 0,
    .optional = true,
};

/* distributor registers */
#define GICD_BASE           (defualt_gicv3_config_desc.mmio_base + defualt_gicv3_config_desc.gicd_offset)

#define GICD_CTLR           (GICD_BASE + 0x0000)
#define GICD_TYPER          (GICD_BASE + 0x0004)
#define GICD_IIDR           (GICD_BASE + 0x0008)
#define GICD_IGROUPR(n)     (GICD_BASE + 0x0080 + (n)*4)
#define GICD_ISENABLER(n)   (GICD_BASE + 0x0100 + (n)*4)
#define GICD_ICENABLER(n)   (GICD_BASE + 0x0180 + (n)*4)
#define GICD_ISPENDR(n)     (GICD_BASE + 0x0200 + (n)*4)
#define GICD_ICPENDR(n)     (GICD_BASE + 0x0280 + (n)*4)
#define GICD_ISACTIVER(n)   (GICD_BASE + 0x0300 + (n)*4)
#define GICD_ICACTIVER(n)   (GICD_BASE + 0x0380 + (n)*4)
#define GICD_IPRIORITYR(n)  (GICD_BASE + 0x0400 + (n)*4)
#define GICD_ITARGETSR(n)   (GICD_BASE + 0x0800 + (n)*4)
#define GICD_ICFGR(n)       (GICD_BASE + 0x0c00 + (n)*4)
#define GICD_IGRPMODR(n)    (GICD_BASE + 0x0d00 + (n)*4)
#define GICD_NSACR(n)       (GICD_BASE + 0x0e00 + (n)*4)
#define GICD_SGIR           (GICD_BASE + 0x0f00)
#define GICD_CPENDSGIR(n)   (GICD_BASE + 0x0f10 + (n)*4)
#define GICD_SPENDSGIR(n)   (GICD_BASE + 0x0f20 + (n)*4)
#define GICD_IROUTER(n)     (GICD_BASE + 0x6000 + (n)*8)
/* GICD_CTLR bit definitions */
#define CTLR_ENABLE_G0      BIT(0)
#define CTLR_ENABLE_G1NS    BIT(1)
#define CTLR_ENABLE_G1S     BIT(2)
#define CTLR_RES0           BIT(3)
#define CTLR_ARE_S          BIT(4)
#define CTLR_ARE_NS         BIT(5)
#define CTLR_DS             BIT(6)
#define CTLR_E1NWF          BIT(7)
#define GICD_CTLR_RWP       BIT(31)
/* peripheral identification registers */
#define GICD_CIDR0 (GICD_BASE + 0xfff0)
#define GICD_CIDR1 (GICD_BASE + 0xfff4)
#define GICD_CIDR2 (GICD_BASE + 0xfff8)
#define GICD_CIDR3 (GICD_BASE + 0xfffc)
#define GICD_PIDR0 (GICD_BASE + 0xffe0)
#define GICD_PIDR1 (GICD_BASE + 0xffe4)
#define GICD_PIDR2 (GICD_BASE + 0xffe8)
#define GICD_PIDR3 (GICD_BASE + 0xffec)

#define GICR_STRIDE         (defualt_gicv3_config_desc.gicr_stride)

/* redistributor registers */
#define GICR_BASE           (defualt_gicv3_config_desc.mmio_base + defualt_gicv3_config_desc.gicr_offset)

#define GICR_SGI_OFFSET     (GICR_BASE + 0x10000)

#define GICR_CTLR(i)        (GICR_BASE + GICR_STRIDE * (i) + 0x0000)
#define GICR_IIDR(i)        (GICR_BASE + GICR_STRIDE * (i) + 0x0004)
#define GICR_TYPER(i, n)    (GICR_BASE + GICR_STRIDE * (i) + 0x0008 + (n)*4)
#define GICR_STATUSR(i)     (GICR_BASE + GICR_STRIDE * (i) + 0x0010)
#define GICR_WAKER(i)       (GICR_BASE + GICR_STRIDE * (i) + 0x0014)
#define GICR_IGROUPR0(i)    (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0080)
#define GICR_IGRPMOD0(i)    (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0d00)
#define GICR_ISENABLER0(i)  (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0100)
#define GICR_ICENABLER0(i)  (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0180)
#define GICR_ISPENDR0(i)    (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0200)
#define GICR_ICPENDR0(i)    (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0280)
#define GICR_ISACTIVER0(i)  (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0300)
#define GICR_ICACTIVER0(i)  (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0380)
#define GICR_IPRIORITYR0(i) (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0400)
#define GICR_ICFGR0(i)      (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0c00)
#define GICR_ICFGR1(i)      (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0c04)
#define GICR_NSACR(i)       (GICR_SGI_OFFSET + GICR_STRIDE * (i) + 0x0e00)

/* GICv4 revision as reported by the PIDR register */
#define ARCH_REV_GICV4 0x4
/* GICv3 revision as reported by the PIDR register */
#define ARCH_REV_GICV3 0x3
/* GICv2 revision as reported by the PIDR register */
#define ARCH_REV_GICV2 0x2
/* GICv1 revision as reported by the PIDR register */
#define ARCH_REV_GICV1 0x1

#define GIC_PRI_LOWEST     0xf0
#define GIC_PRI_IRQ        0xa0
#define GIC_PRI_HIGHEST    0x80 /* Higher priorities belong to Secure-World */

#ifdef CONFIG_ARCH_64
#define MPIDR_AFF0(x) (x & 0xff)
#define MPIDR_AFF1(x) ((x >> 8) & 0xff)
#define MPIDR_AFF2(x) ((x >> 16) & 0xff)
#define MPIDR_AFF3(x) ((x >> 32) & 0xff)
#else
#define MPIDR_AFF0(x) (x & 0xff)
#define MPIDR_AFF1(x) ((x >> 8) & 0xff)
#define MPIDR_AFF2(x) ((x >> 16) & 0xff)
#define MPIDR_AFF3(x) (0)
#endif
#define MPIDR_MT(x)   (x & BIT(24))
#define MPIDR_AFF_MASK(x) (x & 0xff00ffffff)

#define GICC_SRE_EL1_SRE             BIT(0)

#define GICR_WAKER_ProcessorSleep    BIT(1)
#define GICR_WAKER_ChildrenAsleep    BIT(2)

#define GICC_CTLR_EL1_EOImode_drop   BIT(1)

#define DEFAULT_PMR_VALUE            0xff

#define ICC_CTLR_EL1    "S3_0_C12_C12_4"
#define ICC_PMR_EL1     "S3_0_C4_C6_0"
#define ICC_IAR1_EL1    "S3_0_C12_C12_0"
#define ICC_SRE_EL1     "S3_0_C12_C12_5"
#define ICC_BPR1_EL1    "S3_0_C12_C12_3"
#define ICC_IGRPEN1_EL1 "S3_0_C12_C12_7"
#define ICC_EOIR1_EL1   "S3_0_C12_C12_1"
#define ICC_DIR_EL1     "S3_0_C12_C11_1"
#define ICC_SGI1R_EL1   "S3_0_C12_C11_5"

#define ICC_IAR0_EL1    "S3_0_C12_C8_0"
#define ICC_BPR0_EL1    "S3_0_C12_C8_3"
#define ICC_IGRPEN0_EL1 "S3_0_C12_C12_6"
#define ICC_EOIR0_EL1   "S3_0_C12_C8_1"

/* Virt control registers */
#define ICH_AP0R0_EL2   "S3_4_C12_C8_0"
#define ICH_AP0R1_EL2   "S3_4_C12_C8_1"
#define ICH_AP0R2_EL2   "S3_4_C12_C8_2"
#define ICH_AP0R3_EL2   "S3_4_C12_C8_3"
#define ICH_AP1R0_EL2   "S3_4_C12_C9_0"
#define ICH_AP1R1_EL2   "S3_4_C12_C9_1"
#define ICH_AP1R2_EL2   "S3_4_C12_C9_2"
#define ICH_AP1R3_EL2   "S3_4_C12_C9_3"
#define ICH_HCR_EL2     "S3_4_C12_C11_0"
#define ICH_VTR_EL2     "S3_4_C12_C11_1"
#define ICH_MISR_EL2    "S3_4_C12_C11_2"
#define ICH_EISR_EL2    "S3_4_C12_C11_3"
#define ICH_ELRSR_EL2   "S3_4_C12_C11_5"
#define ICH_VMCR_EL2    "S3_4_C12_C11_7"
#define ICH_LR0_EL2     "S3_4_C12_C12_0"
#define ICH_LR1_EL2     "S3_4_C12_C12_1"
#define ICH_LR2_EL2     "S3_4_C12_C12_2"
#define ICH_LR3_EL2     "S3_4_C12_C12_3"
#define ICH_LR4_EL2     "S3_4_C12_C12_4"
#define ICH_LR5_EL2     "S3_4_C12_C12_5"
#define ICH_LR6_EL2     "S3_4_C12_C12_6"
#define ICH_LR7_EL2     "S3_4_C12_C12_7"
#define ICH_LR8_EL2     "S3_4_C12_C13_0"
#define ICH_LR9_EL2     "S3_4_C12_C13_1"
#define ICH_LR10_EL2    "S3_4_C12_C13_2"
#define ICH_LR11_EL2    "S3_4_C12_C13_3"
#define ICH_LR12_EL2    "S3_4_C12_C13_4"
#define ICH_LR13_EL2    "S3_4_C12_C13_5"
#define ICH_LR14_EL2    "S3_4_C12_C13_6"
#define ICH_LR15_EL2    "S3_4_C12_C13_7"

#define INTID_MASK  bitmask(24)

static uint32_t gic_max_int;

static void gicr_wait_rwp(void)
{
    uint32_t cnt = 10000;
    uint32_t cpu = 0;
    while (bitfield_get(getreg32(GICR_CTLR(cpu)), 31, 1)) {
        cnt--;
        if (!cnt) {
            kprintf("gic_wait_rwp timeout!\n");
            return ;
        }
    }
}

static void gicd_wait_rwp(void)
{
    uint32_t cnt = 10000;
    while (bitfield_get(getreg32(GICD_CTLR), 31, 1)) {
        cnt--;
        if (!cnt) {
            kprintf("gic_wait_rwp timeout!\n");
            return ;
        }
    }
}

static inline uint64_t get_current_mpidr(void)
{
    uint64_t mpidr;
    MRS("mpidr_el1", mpidr);
    kprintf("mpidr_el1 %p\n", mpidr);
    return mpidr;
}

static inline uint64_t mpidr_to_gic_affinity(void)
{
    uint64_t mpidr = get_current_mpidr();
    uint64_t affinity = 0;
    affinity = (uint64_t)MPIDR_AFF3(mpidr) << 32 | MPIDR_AFF2(mpidr) << 16 |
               MPIDR_AFF1(mpidr) << 8  | MPIDR_AFF0(mpidr);
    return affinity;
}

static void redistributor_init(void)
{
    uint32_t i;
    uint32_t cpu = 0;

    // mark PE online
    uint32_t waker = getreg32(GICR_WAKER(cpu));
    kprintf("waker = %p\n", waker);
    waker &= ~GICR_WAKER_ProcessorSleep;
    kprintf("waker = %p\n", waker);
    putreg32(waker, GICR_WAKER(cpu));
    while (getreg32(GICR_WAKER(cpu)) & GICR_WAKER_ChildrenAsleep) {
        kprintf("GICv3: GICR_WAKER returned non-zero %x\n", waker);
        PANIC();
    }

    /* deactivate SGIs/PPIs */
    putreg32(~0, GICR_ICACTIVER0(cpu));
    gicr_wait_rwp();

    /* set ICFGR0 for SGIs as edge-triggered */
    uint32_t icfgr0 = getreg32(GICR_ICFGR0(cpu));
    kprintf("icfgr0 = %p\n", icfgr0);
    putreg32(0xaaaaaaaa, GICR_ICFGR0(cpu));
    gicr_wait_rwp();

    /* set ICFGR1 for PPIs as level-triggered */
    uint32_t icfgr1 = getreg32(GICR_ICFGR1(cpu));
    kprintf("icfgr1 = %p\n", icfgr1);
    putreg32(0, GICR_ICFGR1(cpu));
    gicr_wait_rwp();

    /* set priority on PPI and SGI interrupts */
    uint32_t priority = (GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 | GIC_PRI_IRQ << 8 |
                GIC_PRI_IRQ);
    for (i = 0; i < 32; i += 4) {
        putreg32(priority, GICR_IPRIORITYR0(cpu) + i);
    }
    gicr_wait_rwp();

    // redistributer config: configure sgi/ppi as non-secure group 1.
    putreg32(~0, GICR_IGROUPR0(cpu));
    gicr_wait_rwp();

#ifdef CONFIG_ARM_PMU
#ifdef CONFIG_PMU_PSEUDO_NMI
    #define NMI_PRIORITY GIC_PRI_HIGHEST

    u8 *priority_base = GICR_IPRIORITYR0(cpu) + IRQN_PMU;
    putreg8(NMI_PRIORITY, priority_base);
    kprintf("priority_base = %p\n", priority_base);

    u32 *gicr_ipriorityr_base = GICR_IPRIORITYR0(cpu) + (IRQN_PMU / 4) * 4;
    kprintf("GICR_IPRIORITYR0(cpu) = %p, gicr_ipriorityr_base = %p\n", GICR_IPRIORITYR0(cpu), gicr_ipriorityr_base);

    u32 ipriorityr_nmi = getreg32(gicr_ipriorityr_base);
    kprintf("ipriorityr_nmi = %p\n", ipriorityr_nmi);
#endif

#ifdef CONFIG_PMU_FIQ
   // pmu group 0
   putreg32(0xffffffff & ~bit(IRQN_PMU), GICR_IGROUPR0(cpu));
   gicr_wait_rwp();
#endif
#endif

    uint32_t igrp0 = getreg32(GICR_IGROUPR0(cpu));
    kprintf("igrp0 = %p\n", igrp0);
    
    // redistributer config: clear and mask sgi/ppi.
    putreg32(~0, GICR_ICENABLER0(cpu));
    putreg32(~0, GICR_ICPENDR0(cpu));
    gicr_wait_rwp();

    // TODO lpi init
}

static void cpu_interface_init(void)
{
    uint32_t sre;
    // enable system register access
    MRS(ICC_SRE_EL1, sre);
    kprintf("sre = %p\n", sre);
    sre |= GICC_SRE_EL1_SRE;
    kprintf("sre = %p\n", sre);
    MSR(ICC_SRE_EL1, sre);

    // no priority grouping 1: ICC_BPR1_EL1
    MSR(ICC_BPR1_EL1, 0);

    // no priority grouping 0: ICC_BPR0_EL1
    MSR(ICC_BPR0_EL1, 0);

    // set priority mask register: ICC_PMR_EL
    MSR(ICC_PMR_EL1, DEFAULT_PMR_VALUE);

    // EOI drops priority and deactivates the interrupt: ICC_CTLR_EL1
    uint32_t icc_ctlr;
    MRS(ICC_CTLR_EL1, icc_ctlr);
    kprintf("icc_ctlr = %p\n", icc_ctlr);
    icc_ctlr &= ~GICC_CTLR_EL1_EOImode_drop;
    kprintf("icc_ctlr = %p\n", icc_ctlr);
    MSR(ICC_CTLR_EL1, icc_ctlr);

    // Enable Group1 interrupts: ICC_IGRPEN1_EL1
    MSR(ICC_IGRPEN1_EL1, 1);

    // Enable Group0 interrupts: ICC_IGRPEN0_EL1
    MSR(ICC_IGRPEN0_EL1, 1);

    // Sync at once at the end of cpu interface configuration
    isb();
    dsb();
}

static void distributor_init(void)
{
    uint32_t i;

    // read gic version
    uint32_t pidr2 = getreg32(GICD_PIDR2);
    uint32_t gic_version = bitfield_get(pidr2, 4, 4);
    assert((gic_version == ARCH_REV_GICV3) || (gic_version == ARCH_REV_GICV4));

    // read gic max interrupts
    uint32_t typer = getreg32(GICD_TYPER);
    gic_max_int = (bitfield_get(typer, 0, 5) + 1) * 32;
    kprintf("gic_version = %d, gic_max_int = %d, typer = %p\n", gic_version, gic_max_int, typer);

    // disable distributor
    putreg32(0, GICD_CTLR);
    gicd_wait_rwp();

    // assume level-trigger
    for (i = 32; i < gic_max_int; i += 16) {
        putreg32(0, GICD_ICFGR(i / 16));
    }

    // default priority for global interrupts
    uint32_t priority = (GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 | GIC_PRI_IRQ << 8 |
                GIC_PRI_IRQ);
    for (i = 32; i < gic_max_int; i += 4) {
        putreg32(priority, GICD_IPRIORITYR(i / 4));
    }

    // distributor config: mask and clear all spis, set group 1.
    for (i = 32; i < gic_max_int; i += 32) {
        putreg32(~0, GICD_ICENABLER(i / 32));
        putreg32(~0, GICD_ICPENDR(i / 32));
        putreg32(~0, GICD_IGROUPR(i / 32));
        putreg32(0, GICD_IGRPMODR(i / 32));
    }
    gicd_wait_rwp();
    
    // enable distributor with ARE, group 1 enable
    putreg32(CTLR_ENABLE_G0 | CTLR_ENABLE_G1NS | CTLR_ARE_S, GICD_CTLR);
    gicd_wait_rwp();

    // route all global IRQs to this CPU
    uint64_t affinity = mpidr_to_gic_affinity();
    kprintf("affinity = %p\n", affinity);
    for (i = 32; i < gic_max_int; i++) {
        putreg64(affinity, GICD_IROUTER(i));
    }
    gicd_wait_rwp();

    isb();
    dsb();
}

/****************************************************************************
 * Name: gic_initialize
 *
 * Description:
 *   Perform common GIC initialization for the current CPU (all CPUs)
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void gic_initialize(void)
{
    // init distributor
    distributor_init();

    // init redistributor
    redistributor_init();

    // init cpu interface
    cpu_interface_init();
}

void decode_fiq(void)
{
    kprintf("decode_fiq\n");

    uint32_t fiq;
    uint64_t icciar0;

    timer_update_timestamp();

    MRS(ICC_IAR0_EL1, icciar0);
    //kprintf("ICC_IAR0_EL1 = %p\n", icciar0);
    fiq = icciar0 & INTID_MASK;

    /* Ignore spurions IRQs.  ICC_IAR0_EL1 will report 1023 if there is no pending
     * interrupt.
     */
    assert(fiq < NR_IRQS || fiq == 1023);
    if (fiq< NR_IRQS)
    {
        /* Dispatch the interrupt */
        irq_dispatch(fiq);
    }

    /* Write to the end-of-interrupt register */
    MSR(ICC_EOIR0_EL1, icciar0);

    // 恢复当前任务的上下文
    //kprintf("restore_current_context\n");
    restore_current_context();
}

/****************************************************************************
 * Name: decode_irq
 *
 * Description:
 *   This function is called from the IRQ vector handler in vector.S.
 *   At this point, the interrupt has been taken and the registers have
 *   been saved on the stack.  This function simply needs to determine the
 *   the irq number of the interrupt and then to call doirq to dispatch
 *   the interrupt.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void decode_irq(unsigned long elr, unsigned long spsr)
{
    uint32_t irq;
    uint64_t icciar1;

    //kprintf("%s:%d# elr = %p, spsr = %p\n", __FUNCTION__, __LINE__, elr, spsr);

#if 0
    u64 bench_cnt = bench_get_counter(0);
    kprintf("3.bench_cnt = %p\n", bench_cnt); 

    u64 cycle_count = bench_get_cycle_count();
    kprintf("cycle_count = %lu\n", cycle_count);
#endif

    timer_update_timestamp();

    MRS(ICC_IAR1_EL1, icciar1);
    //kprintf("ICC_IAR1_EL1 = %p\n", icciar1);
    irq = icciar1 & INTID_MASK;

#if 0
    if (irq == IRQN_PMU) {
        kprintf("IRQN_PMU\n");
    } else {
        kprintf("IRQN_%d\n", irq);
        // 中断嵌套触发测试
        // force trigger pmu interrupt overflow
        MSR("PMSELR_EL0", 0);
        MSR("PMXEVCNTR_EL0", 0xfff00000);

    }

    MSR(ICC_PMR_EL1, GIC_PRI_IRQ);
    arch_local_irq_enable();
    kprintf("%s:%d\n", __FUNCTION__, __LINE__);
#endif
    
    /* Ignore spurions IRQs.  ICC_IAR1_EL1 will report 1023 if there is no pending
     * interrupt.
     */
    assert(irq < NR_IRQS || irq == 1023);
    if (irq < NR_IRQS)
    {
        /* Dispatch the interrupt */
        irq_dispatch(irq);
    }

    /* Write to the end-of-interrupt register */
    MSR(ICC_EOIR1_EL1, icciar1);

#if 0
    arch_local_irq_disable();
    MSR(ICC_PMR_EL1, DEFAULT_PMR_VALUE);
#endif

    // 恢复当前任务的上下文
    //kprintf("restore_current_context\n");
    restore_current_context();
}

/****************************************************************************
 * Name: up_enable_irq
 *
 * Description:
 *   On many architectures, there are three levels of interrupt enabling: (1)
 *   at the global level, (2) at the level of the interrupt controller,
 *   and (3) at the device level.  In order to receive interrupts, they
 *   must be enabled at all three levels.
 *
 *   This function implements enabling of the device specified by 'irq'
 *   at the interrupt controller level if supported by the architecture
 *   (up_irq_restore() supports the global level, the device level is
 *   hardware specific).
 *
 *   Since this API is not supported on all architectures, it should be
 *   avoided in common implementations where possible.
 *
 ****************************************************************************/
void up_enable_irq(uint32_t irq)
{
    uint32_t mask;

    mask = 1 << (irq % 32);
    if (irq >= IRQN_SGI0 && irq < IRQN_SPIs) {
        putreg32(mask, GICR_ISENABLER0(0));
        gicr_wait_rwp();
    } else if (irq >= IRQN_SPIs && irq < gic_max_int) {
        putreg32(mask, GICD_ISENABLER(irq / 32));
        gicd_wait_rwp();
    } else {
        kprintf("unsupported irq %d\n", irq);
    }
}

/****************************************************************************
 * Name: up_disable_irq
 *
 * Description:
 *   This function implements disabling of the device specified by 'irq'
 *   at the interrupt controller level if supported by the architecture
 *   (up_irq_save() supports the global level, the device level is hardware
 *   specific).
 *
 *   Since this API is not supported on all architectures, it should be
 *   avoided in common implementations where possible.
 *
 ****************************************************************************/
void up_disable_irq(uint32_t irq)
{
    uint32_t mask;
    uint32_t cpu = 0;

    mask = 1 << (irq % 32);
    if (irq >= IRQN_SGI0 && irq < IRQN_SPIs) {
        putreg32(mask, GICR_ICENABLER0(cpu));
        gicr_wait_rwp();
    } else if (irq >= IRQN_SPIs && irq < gic_max_int) {
        putreg32(mask, GICD_ICENABLER(irq / 32));
        gicd_wait_rwp();
    } else {
        kprintf("unsupported irq %d\n", irq);
    }
}

void up_ack_irq(uint32_t irq)
{
}

