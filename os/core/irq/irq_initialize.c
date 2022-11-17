#include <k_stdio.h>
#include <k_stddef.h>
#include <uapi/list.h>

#include <irq.h>

struct irq_info g_irqvector[NR_IRQS];

/****************************************************************************
 * Name: irq_initialize
 *
 * Description:
 *   Configure the IRQ subsystem
 *
 ****************************************************************************/
void irq_initialize(void)
{
    int i;

    /* Point all interrupt vectors to the unexpected interrupt */
    for (i = 0; i < NR_IRQS; i++)
    {
        //g_irqvector[i].handler = irq_unexpected_isr;
        g_irqvector[i].handler = NULL;
        g_irqvector[i].arg     = NULL;
        g_irqvector[i].count   = 0;
    }

    gic_initialize();
}

void irq_enable(uint32_t irq)
{
    up_enable_irq(irq);
}

void irq_disable(uint32_t irq)
{
    up_disable_irq(irq);
}

