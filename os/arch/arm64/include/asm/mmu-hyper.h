#ifndef __ASM_MMU_HYPER_H
#define __ASM_MMU_HYPER_H

#include <chinos/config.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT

#ifndef __ASSEMBLY__

#include <addrspace.h>

extern struct addrspace hyper_kernel_addrspace;
extern struct addrspace hyper_user_addrspace;

void hyper_as_switch(struct addrspace *as, unsigned long type, asid_t asid);
void hyper_as_initialize(void);
int  hyper_as_map(struct addrspace *as, struct mem_region *region, uint32_t prot, RAM_TYPE_e type);
int  _hyper_as_map(struct addrspace *as, struct mem_region *region, uint32_t prot, RAM_TYPE_e type);

#endif /* !__ASSEMBLY__ */

#endif

#endif
