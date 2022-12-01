#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_time.h>
#include <irq.h>
#include <generic_timer.h>
#include <scheduler.h>

#include "timer.h"

volatile time_t g_current_time;
static struct timer g_timer;

static int timer_handler(int irq, void *context, void *arg)
{
#if 0
    static int timer_irq_cnts = 0;
    printf("timer_handler %d\n", timer_irq_cnts++);
#endif

#ifdef TIMER_DEADLINE
    set_deadline(g_current_time + CONFIG_RR_INTERVAL * TICKS_PER_MS);
#else
    generic_timer_clear();
    generic_timer_reset(CONFIG_RR_INTERVAL);
#endif

#if 0
    static volatile time_t next;
    time_t consumed = g_current_time - next;
    kprintf("g_system_timer = %lld, %lld\n", g_current_time, consumed - TICKS_PER_MS*CONFIG_RR_INTERVAL);
    next = g_current_time;
#endif

    schedule();

    return 0;
}

void timer_update_timestamp(void)
{
    g_current_time = get_current_time();
}

void timer_reset(uint32_t timeslice)
{
    generic_timer_reset(timeslice);
}

void timer_init(void)
{
    generic_timer_init(&g_timer);

    irq_attach(g_timer.irq, timer_handler, NULL, NULL);
    irq_enable(g_timer.irq);
}

unsigned long sys_nanosleep(unsigned long req, unsigned long rem)
{
    //kprintf("sys_nanosleep: req = %p, rem = %p\n", req, rem);
    struct timespec *tv = (struct timespec *)req;
    //kprintf("timespec tv.tv_sec = %p, tv.tv_nsec = %p\n", tv->tv_sec, tv->tv_nsec);

    struct tcb *current = this_task();

    current->readytime = get_current_time() + tv->tv_sec * TICKS_PER_S + tv->tv_nsec * TICKS_PER_NS;

    sched_sleep(current);

    return 0;
}
