#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#include <uapi/vsyscall.h>

#define SYSCALLS_NUM __NR_landlock_restrict_self

typedef unsigned long (*sys_callback)(unsigned long *params);

extern sys_callback svc_handlers[SYSCALLS_NUM];

#endif
