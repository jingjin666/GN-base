#include <k_stdio.h>
#include <k_stddef.h>
#include <k_assert.h>
#include <mmap.h>
#include <timer.h>
#include <task.h>

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

static unsigned long vsys_thread_create(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;

    kprintf("p0 = %p, p1 = %p, p2 = %p, p3 = %p, p4 = %p, p5 = %p\n", p0, p1, p2, p3, p4, p5);

    unsigned long msg_len = p0;
    unsigned long msg[5];

    if (msg_len > 5*sizeof(long)) {
        return -1;
    }

    msg[0] = p1;
    msg[1] = p2;

    kprintf("msg_len = %p, msg = [%p, %p]\n", msg_len, msg[0], msg[1]);

    unsigned long entry = msg[0];
    unsigned long stack = msg[1];

    return sys_thread_create(entry, stack);
}

#ifdef CONFIG_HYPERVISOR_SUPPORT
static unsigned long vsys_vcpu_create(unsigned long *params)
{
    unsigned long p0 = *params++;
    unsigned long p1 = *params++;
    unsigned long p2 = *params++;
    unsigned long p3 = *params++;
    unsigned long p4 = *params++;
    unsigned long p5 = *params++;

    kprintf("p0 = %p, p1 = %p, p2 = %p, p3 = %p, p4 = %p, p5 = %p\n", p0, p1, p2, p3, p4, p5);

    unsigned long msg_len = p0;
    unsigned long msg[5];

    if (msg_len > 5*sizeof(long)) {
        return -1;
    }

    msg[0] = p1;
    msg[1] = p2;
    msg[2] = p3;
    msg[3] = p4;

    kprintf("msg_len = %p, msg = [%p, %p, %p, %p]\n", msg_len, msg[0], msg[1], msg[2], msg[3]);

    unsigned long entry = msg[0];
    unsigned long stack = msg[1];
    unsigned long vm_base = msg[2];
    unsigned long vm_size = msg[3];

    return sys_vcpu_create(entry, stack, vm_base, vm_size);
}
#endif

sys_callback svc_handlers[SYSCALLS_NUM] = {
    [__NR_ioctl]        = vsys_ioctl,
    [__NR_writev]       = vsys_writev,
    [__NR_brk]          = vsys_brk,
    [__NR_mmap]         = vsys_mmap,
    [__NR_munmap]       = vsys_munmap,
    [__NR_exit]         = vsys_exit,
    [__NR_exit_group]   = vsys_exit_group,
    [__NR_nanosleep]    = vsys_nanosleep,
    [__NR_thread_create]= vsys_thread_create,
#ifdef CONFIG_HYPERVISOR_SUPPORT
    [__NR_vcpu_create]  = vsys_vcpu_create,
#endif
};

