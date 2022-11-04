#ifndef __GIC_H
#define __GIC_H

#include <uapi/types.h>

#define GIC_IRQ_SGI                     0

#define GIC_IRQ_PPI                     16

#define GIC_IRQ_HYPER_PHYS_TIMER        26
#define GIC_IRQ_VIRT_TIMER              27
#define GIC_IRQ_HYPER_VIRT_TIMER        28
#define GIC_IRQ_SECURE_PHYS_TIMER       29
#define GIC_IRQ_NON_SECURE_PHYS_TIMER   30

#define GIC_IRQ_SPI                     32

#define GIC_IRQ_LPI                     8192

enum irq_numbers
{
    IRQN_SGI0 = 0,              // SGI 0
    IRQN_SGI1 = 1,              // SGI 1
    IRQN_SGI2 = 2,              // SGI 2
    IRQN_SGI3 = 3,              // SGI 3
    IRQN_SGI4 = 4,              // SGI 4
    IRQN_SGI5 = 5,              // SGI 5
    IRQN_SGI6 = 6,              // SGI 6
    IRQN_SGI7 = 7,              // SGI 7
    IRQN_SGI8 = 8,              // SGI 8
    IRQN_SGI9 = 9,              // SGI 9
    IRQN_SGI10 = 10,            // SGI 10
    IRQN_SGI11 = 11,            // SGI 11
    IRQN_SGI12 = 12,            // SGI 12
    IRQN_SGI13 = 13,            // SGI 13
    IRQN_SGI14 = 14,            // SGI 14
    IRQN_SGI15 = 15,            // SGI 15
    // Private Peripheral Interrupts
    IRQN_PPI_16 = 16,
    IRQN_PPI_17 = 17,
    IRQN_PPI_18 = 18,
    IRQN_PPI_19 = 19,
    IRQN_PPI_20 = 20,
    IRQN_PPI_21 = 21,
    IRQN_PPI_22 = 22,
    IRQN_PPI_23 = 23,
    IRQN_PPI_24 = 24,
    IRQN_PPI_25 = 25,
    IRQN_PPI_26 = 26, IRQN_HYPER_PHYS_TIMER = IRQN_PPI_26,
    IRQN_PPI_27 = 27, IRQN_VIRT_TIMER = IRQN_PPI_27,
    IRQN_PPI_28 = 28, IRQN_HYPER_VIRT_TIMER = IRQN_PPI_28,
    IRQN_PPI_29 = 29, IRQN_SECURE_PHYS_TIMER = IRQN_PPI_29,
    IRQN_PPI_30 = 30, IRQN_NON_SECURE_PHYS_TIMER = IRQN_PPI_30,
    IRQN_PPI_31 = 31,
    // Shared Peripheral interrupts
    IRQN_SPIs = 32,
    // Locality-specific Peripheral interrupts
    //IRQN_LPIs = 8192,
    // Total number of interrupts
    NR_IRQS
};

typedef unsigned long irqstate_t;

static inline irqstate_t irqstate(void)
{
    return 0;
}

static inline irqstate_t local_irq_save(void)
{
    return 0;
}

static inline void local_irq_restore(irqstate_t flags)
{

}

static inline void arch_local_irq_enable(void)
{
	asm volatile
    (
		"msr	daifclr, #2"
		:
		:
		: "memory"
    );
}

static inline void arch_local_irq_disable(void)
{
	asm volatile
    (
		"msr	daifset, #2"
		:
		:
		: "memory"
    );
}

void decode_irq(u64 sp);
void gic_initialize(void);
void up_enable_irq(u32 irq);
void up_disable_irq(u32 irq);
void up_ack_irq(u32 irq);
#endif
