#include <k_stdio.h>
#include <k_stdint.h>
#include <k_assert.h>
#include <mmap.h>

#include "syscalls.h"

unsigned long vsys_ioctl(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;
    unsigned long p6 = *params++;
    unsigned long p7 = *params++;

    return 0;
}

unsigned long vsys_writev(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;
    unsigned long p6 = *params++;
    unsigned long p7 = *params++;

    kprintf("p0 = %p, p1 = %p, p2 = %p, p3 = %p, p4 = %p, p5 = %p, p6 = %p, p7 = %p\n", p0, p1, p2, p3, p4, p5, p6, p7);

    return p2;
}

unsigned long vsys_brk(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;
    unsigned long p6 = *params++;
    unsigned long p7 = *params++;

    kprintf("p0 = %p, p1 = %p, p2 = %p, p3 = %p, p4 = %p, p5 = %p, p6 = %p, p7 = %p\n", p0, p1, p2, p3, p4, p5, p6, p7);

    return sys_brk(p0);
}

unsigned long vsys_mmap(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;
    unsigned long p6 = *params++;
    unsigned long p7 = *params++;

    kprintf("p0 = %p, p1 = %p, p2 = %p, p3 = %p, p4 = %p, p5 = %p, p6 = %p, p7 = %p\n", p0, p1, p2, p3, p4, p5, p6, p7);
    return sys_mmap(p0, p1, p2, p3, p4, p5);
}

