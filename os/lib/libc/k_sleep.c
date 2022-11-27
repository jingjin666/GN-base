#include <chinos/config.h>

#include <k_stdint.h>
#include <k_unistd.h>
#if 0
#include <uapi/syscall.h>

int k_sleep(uint32_t seconds)
{
    return k_usleep(seconds * 1000000);
}

int k_usleep(uint32_t usec)
{
    return sys_call1(SYS_nanosleep, usec * 1000);
}

int k_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    return sys_call1(SYS_nanosleep, rqtp->tv_nsec);
}

int k_yield(void)
{
    return sys_call0(SYS_yield);
}

#endif
