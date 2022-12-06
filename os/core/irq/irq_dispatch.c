#include <k_stdio.h>
#include <k_stddef.h>
#include <k_assert.h>

#include <irq.h>

/****************************************************************************
 * Name: irq_unexpected_isr
 *
 * Description:
 *   An interrupt has been received for an IRQ that was never registered
 *   with the system.
 *
 ****************************************************************************/
int irq_unexpected_isr(int irq, void *context, void *arg)
{
    local_irq_save();
    kprintf("ERROR irq: %d\n", irq);
    PANIC();
    return 0; /* Won't get here */
}

/****************************************************************************
 * Name: irq_dispatch
 *
 * Description:
 *   This function must be called from the architecture-specific logic in
 *   order to dispatch an interrupt to the appropriate, registered handling
 *   logic.
 *
 ****************************************************************************/
void irq_dispatch(uint32_t irq)
{
    uint32_t ndx = irq;
    void *arg = NULL;
    //xcpt_t vector = irq_unexpected_isr;
    xcpt_t vector = NULL;

    if (ndx >= NR_IRQS) {
        return ;
    }

    if (g_irqvector[ndx].handler) {
        vector = g_irqvector[ndx].handler;
        arg = g_irqvector[ndx].arg;
    }

    g_irqvector[ndx].count++;

    if (vector) {
        //kprintf("kernel irq handle %d\n", irq);
        vector(irq, NULL, arg);
    }
}

