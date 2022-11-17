#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <irq.h>
#include <generic_timer.h>

#include "timer.h"

static struct timer g_timer;

static int timer_handler(int irq, void *context, void *arg)
{
    static int timer_irq_cnts = 0;
    kprintf("timer_handler %d\n", timer_irq_cnts++);

    generic_timer_clear();

    schedule();

    generic_timer_reset(CONFIG_RR_INTERVAL*1000);

    return 0;
}

void timer_reset(uint32_t timeslice)
{
    generic_timer_reset(timeslice * 1000);
}

void timer_init(void)
{
    generic_timer_init(&g_timer);

    irq_attach(g_timer.irq, timer_handler, NULL, NULL);
    irq_enable(g_timer.irq);
}
