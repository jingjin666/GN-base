#ifndef __ADDRSPACE_H
#define __ADDRSPACE_H

#include <chinos/config.h>

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

extern struct addrspace kernel_addrspace;
extern struct addrspace user_addrspace;

void as_switch(struct addrspace *as, unsigned long type);
void as_initialize(void);
int  as_map(struct addrspace *as, struct mem_region *region, uint32_t prot, RAM_TYPE_e type);
#endif
