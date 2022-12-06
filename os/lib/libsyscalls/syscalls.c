#include <k_stdio.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <mmap.h>
#include <timer.h>

#include "syscalls.h"

static unsigned long vsys_ioctl(unsigned long *params)
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

    return 0;
}

static unsigned long vsys_writev(unsigned long *params)
{
    // todo
    return 0;
}

static unsigned long vsys_brk(unsigned long *params)
{
    unsigned long p0 = *params++;

    return sys_brk(p0);
}

static unsigned long vsys_mmap(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;

    return sys_mmap(p0, p1, p2, p3, p4, p5);
}

static unsigned long vsys_munmap(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;

    return sys_munmap(p0, p1);
}

static unsigned long vsys_exit(unsigned long *params)
{
    // todo
    return 0;
}

static unsigned long vsys_exit_group(unsigned long *params)
{
    // todo
    return 0;
}

static unsigned long vsys_nanosleep(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;

    return sys_nanosleep(p0, p1);
}

sys_callback svc_handlers[SYSCALLS_NUM] = {
    [__NR_ioctl]        = vsys_ioctl,
    [__NR_writev]       = vsys_writev,
    [__NR_brk]          = vsys_brk,
    [__NR_mmap]         = vsys_mmap,
    [__NR_munmap]       = vsys_munmap,
    [__NR_exit]         = vsys_exit,
    [__NR_exit_group]   = vsys_exit_group,
    [__NR_nanosleep]    = vsys_nanosleep,
};
