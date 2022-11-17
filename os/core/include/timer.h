#ifndef __SCHED_TIMER_H
#define __SCHED_TIMER_H

#include <k_stdint.h>

typedef struct timer
{
    uint32_t irq;
    uint32_t clock_freq;
} timer_t;

void timer_init(void);

#endif
