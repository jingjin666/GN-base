#include <chinos/config.h>

#include <k_stdint.h>
#include <k_unistd.h>
#include <uapi/syscall.h>

int k_sleep(uint32_t seconds)
{
    return k_usleep(seconds * 1000000);
}

int k_usleep(uint32_t usec)
{
    return sys_call1(SVC_syscall_usleep, usec);
}

int k_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    // todo
    //return sys_call1(SVC_syscall_nanosleep, rqtp->tv_nsec);
    return 0;
}

void k_yield(void)
{
    sys_call0(SVC_syscall_yield);
}
