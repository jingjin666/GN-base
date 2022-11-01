#ifndef __K_UNISTD_H
#define __K_UNISTD_H

#include <chinos/config.h>

#include <k_stdint.h>
#include <k_time.h>

int k_sleep(uint32_t seconds);
int k_usleep(uint32_t usec);
int k_nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
void k_yield(void);

#endif
