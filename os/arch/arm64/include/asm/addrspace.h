#ifndef __ASM_ADDRSPACE_H
#define __ASM_ADDRSPACE_H

#include <chinos/config.h>

#ifndef __ASSEMBLY__

#include <pagetable.h>

typedef enum RAM_TYPE
{
    RAM_NORMAL = 0,
    RAM_DEVICE,
} RAM_TYPE_e;

typedef struct mem_region
{
    unsigned long pbase;
    unsigned long vbase;
    unsigned long size;
} mem_region_t;

typedef struct addrspace
{
    struct page_table pg_table;
} addrspace_t;

#endif /* !__ASSEMBLY__ */

#endif
