#ifndef __SCHED_TIMER_H
#define __SCHED_TIMER_H

#include <k_types.h>
#include <k_time.h>

typedef struct timer
{
    uint32_t irq;
    uint32_t clock_freq;
} timer_t;

void timer_update_timestamp(void);
void timer_init(void);
unsigned long sys_nanosleep(unsigned long req, unsigned long rem);

#endif
