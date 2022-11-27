#ifndef __SYSCALLS_H
#define __SYSCALLS_H

unsigned long vsys_ioctl(unsigned long *params);
unsigned long vsys_writev(unsigned long *params);
unsigned long vsys_brk(unsigned long *params);
unsigned long vsys_mmap(unsigned long *params);

#endif
