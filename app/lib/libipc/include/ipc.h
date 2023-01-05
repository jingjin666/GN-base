#ifndef __IPC_H
#define __IPC_H

#include <sys/types.h>

void thread_create(unsigned long entry, unsigned long stack);
void vcpu_create(unsigned long entry, unsigned long stack, unsigned long vm_base, unsigned long vm_size);
unsigned long ipc_send(unsigned long msg_id, void *buf, size_t len);

#endif
