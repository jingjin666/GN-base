#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <uapi/errors.h>
#include <irq.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: irq_attach
 *
 * Description:
 *   Configure the IRQ subsystem so that IRQ number 'irq' is dispatched to
 *   'isr'
 *
 ****************************************************************************/
int irq_attach(uint32_t irq, xcpt_t isr, void *arg, unsigned long *notify)
{
    int ret = 0;
    uint32_t ndx = irq;

    if (ndx >= NR_IRQS) {
        return -EINVAL;
    }

    /* Save the new ISR and its argument in the table. */
    g_irqvector[ndx].handler = isr;
    g_irqvector[ndx].arg = arg;
    g_irqvector[ndx].count = 0;

    ret = OK;

    return ret;
}
