#include <k_assert.h>
#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_stdbool.h>
#include <k_assert.h>
#include <uapi/types.h>
#include <uapi/util.h>
#include <barrier.h>
#include <instructionset.h>
#include <board.h>
#include <irq.h>
#include <task.h>
#include <scheduler.h>

#include <gic.h>

struct gicv3_config_desc
{
    u64 mmio_base;
    u64 gicd_offset;
    u64 gicr_offset;
    u64 gicr_stride;
    u64 reserved0;
    u64 ipi_base;
    u64 optional;
    u64 reserved1[3];
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

#define ICC_CTLR_EL1 "S3_0_C12_C12_4"
#define ICC_PMR_EL1 "S3_0_C4_C6_0"
#define ICC_IAR1_EL1 "S3_0_C12_C12_0"
#define ICC_SRE_EL1 "S3_0_C12_C12_5"
#define ICC_BPR1_EL1 "S3_0_C12_C12_3"
#define ICC_IGRPEN1_EL1 "S3_0_C12_C12_7"
#define ICC_EOIR1_EL1 "S3_0_C12_C12_1"
#define ICC_DIR_EL1 "S3_0_C12_C11_1"
#define ICC_SGI1R_EL1 "S3_0_C12_C11_5"

#define INTID_MASK  lowbitsmask(24)

static u32 gic_max_int;

static void gicr_wait_rwp(void)
{
    u32 cnt = 10000;
    u32 cpu = 0;
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
    u32 cnt = 10000;
    while (bitfield_get(getreg32(GICD_CTLR), 31, 1)) {
        cnt--;
        if (!cnt) {
            kprintf("gic_wait_rwp timeout!\n");
            return ;
        }
    }
}

static inline u64 get_current_mpidr(void)
{
    u64 mpidr;
    MRS("mpidr_el1", mpidr);
    kprintf("mpidr_el1 %p\n", mpidr);
    return mpidr;
}

static inline u64 mpidr_to_gic_affinity(void)
{
    u64 mpidr = get_current_mpidr();
    u64 affinity = 0;
    affinity = (uint64_t)MPIDR_AFF3(mpidr) << 32 | MPIDR_AFF2(mpidr) << 16 |
               MPIDR_AFF1(mpidr) << 8  | MPIDR_AFF0(mpidr);
    return affinity;
}

static void redistributor_init(void)
{
    u32 i;
    u32 cpu = 0;

    // mark PE online
    u32 waker = getreg32(GICR_WAKER(cpu));
    kprintf("waker = %p\n", waker);
    waker &= ~GICR_WAKER_ProcessorSleep;
    kprintf("waker = %p\n", waker);
    putreg32(waker, GICR_WAKER(cpu));
    while (getreg32(GICR_WAKER(cpu)) & GICR_WAKER_ChildrenAsleep) {
        kprintf("GICv3: GICR_WAKER returned non-zero %x\n", waker);
        halt();
    }

    /* deactivate SGIs/PPIs */
    putreg32(~0, GICR_ICACTIVER0(cpu));
    gicr_wait_rwp();

    /* set ICFGR0 for SGIs as edge-triggered */
    u32 icfgr0 = getreg32(GICR_ICFGR0(cpu));
    kprintf("icfgr0 = %p\n", icfgr0);
    putreg32(0xaaaaaaaa, GICR_ICFGR0(cpu));
    gicr_wait_rwp();

    /* set ICFGR1 for PPIs as level-triggered */
    u32 icfgr1 = getreg32(GICR_ICFGR1(cpu));
    kprintf("icfgr1 = %p\n", icfgr1);
    putreg32(0, GICR_ICFGR1(cpu));
    gicr_wait_rwp();

    /* set priority on PPI and SGI interrupts */
    u32 priority = (GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 | GIC_PRI_IRQ << 8 |
                GIC_PRI_IRQ);
    for (i = 0; i < 32; i += 4) {
        putreg32(priority, GICR_IPRIORITYR0(cpu) + i);
    }
    gicr_wait_rwp();

    // redistributer config: configure sgi/ppi as non-secure group 1.
    putreg32(~0, GICR_IGROUPR0(cpu));
    gicr_wait_rwp();

    // redistributer config: clear and mask sgi/ppi.
    putreg32(~0, GICR_ICENABLER0(cpu));
    putreg32(~0, GICR_ICPENDR0(cpu));
    gicr_wait_rwp();

    // TODO lpi init
}

static void cpu_interface_init(void)
{
    u32 sre;
    // enable system register access
    MRS(ICC_SRE_EL1, sre);
    kprintf("sre = %p\n", sre);
    sre |= GICC_SRE_EL1_SRE;
    kprintf("sre = %p\n", sre);
    MSR(ICC_SRE_EL1, sre);

    // no priority grouping: ICC_BPR1_EL1
    MSR(ICC_BPR1_EL1, 0);

    // set priority mask register: ICC_PMR_EL
    MSR(ICC_PMR_EL1, DEFAULT_PMR_VALUE);

    // EOI drops priority and deactivates the interrupt: ICC_CTLR_EL1
    u32 icc_ctlr;
    MRS(ICC_CTLR_EL1, icc_ctlr);
    kprintf("icc_ctlr = %p\n", icc_ctlr);
    icc_ctlr &= ~GICC_CTLR_EL1_EOImode_drop;
    kprintf("icc_ctlr = %p\n", icc_ctlr);
    MSR(ICC_CTLR_EL1, icc_ctlr);

    // Enable Group1 interrupts: ICC_IGRPEN1_EL1
    MSR(ICC_IGRPEN1_EL1, 1);

    // Sync at once at the end of cpu interface configuration
    isb();
    dsb();
}

static void distributor_init(void)
{
    u32 i;

    // read gic version
    u32 pidr2 = getreg32(GICD_PIDR2);
    u32 gic_version = bitfield_get(pidr2, 4, 4);
    assert((gic_version == ARCH_REV_GICV3) || (gic_version == ARCH_REV_GICV4));

    // read gic max interrupts
    u32 typer = getreg32(GICD_TYPER);
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
    u32 priority = (GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 | GIC_PRI_IRQ << 8 |
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
    u64 affinity = mpidr_to_gic_affinity();
    kprintf("affinity = %p\n", affinity);
    for (i = 32; i < gic_max_int; i++) {
        putreg64(GICD_IROUTER(i), affinity);
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

void decode_irq(u64 sp)
{
    kprintf("decode_irq\n");

    struct tcb *task = this_task();
    assert(task);
    task->context.regs[SP] = sp;

    u32 irq;
    u64 icciar1;

    MRS(ICC_IAR1_EL1, icciar1);
    //kprintf("ICC_IAR1_EL1 = %p\n", icciar1);
    irq = icciar1 & INTID_MASK;

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

    // 恢复当前任务的上下文
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
void up_enable_irq(u32 irq)
{
    u32 mask;

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
void up_disable_irq(u32 irq)
{
    u32 mask;
    u32 cpu = 0;

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

void up_ack_irq(u32 irq)
{
}

