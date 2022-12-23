#ifndef __ASM_MMU_H
#define __ASM_MMU_H

#include <chinos/config.h>

#ifndef __ASSEMBLY__

#include <addrspace.h>

extern struct addrspace kernel_addrspace;
extern struct addrspace user_addrspace;

void as_switch(struct addrspace *as, unsigned long type, asid_t asid);
void as_initialize(void);
int  as_map(struct addrspace *as, struct mem_region *region, uint32_t prot, RAM_TYPE_e type);

#endif /* !__ASSEMBLY__ */

#endif
