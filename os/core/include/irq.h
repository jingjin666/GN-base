#ifndef __IRQ_H
#define __IRQ_H

#include <k_types.h>
#include <gic.h>

/* This struct defines the form of an interrupt service routine */
typedef int (*xcpt_t)(int irq, void *context, void *arg);

typedef struct irq_info
{
    xcpt_t handler;     /* Address of the interrupt handler */
    void *arg;          /* The argument provided to the interrupt handler. */
    uint64_t count;     /* Number of interrupts on this IRQ */
} irq_info_t;

extern struct irq_info g_irqvector[NR_IRQS];

void irq_initialize(void);
void irq_dispatch(uint32_t irq);
int irq_attach(uint32_t irq, xcpt_t isr, void *arg, unsigned long *notify);
int irq_unexpected_isr(int irq, void *context, void *arg);
void irq_enable(uint32_t irq);
void irq_disable(uint32_t irq);
#endif
