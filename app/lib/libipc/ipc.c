#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include <syscall_arch.h>

#include "ipc.h"

void thread_create(unsigned long entry, unsigned long stack)
{
    unsigned long buff[2];
    buff[0] = entry;
    buff[1] = stack;
    ipc_send(450/*SYS_thread_create*/, (void *)buff, sizeof(buff));
}

void vcpu_create(unsigned long entry, unsigned long stack, unsigned long vm_base, unsigned long vm_size)
{
    unsigned long buff[4];
    buff[0] = entry;
    buff[1] = stack;
    buff[2] = vm_base;
    buff[3] = vm_size;
    ipc_send(451/*SYS_vcpu_create*/, (void *)buff, sizeof(buff));
}

unsigned long ipc_send(unsigned long msg_id, void *buf, size_t len)
{
    if (len <= sizeof(long) * 5) {
        long param[5] = {0};
        memcpy(param, buf, len);
        __syscall6(msg_id, len, param[0], param[1], param[2], param[3], param[4]);
    } else {
        
    }
}

