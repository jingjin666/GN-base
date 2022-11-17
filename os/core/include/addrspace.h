#ifndef __ADDRSPACE_H
#define __ADDRSPACE_H

#include <chinos/config.h>

#include <pagetable.h>

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

void as_switch(struct addrspace *as);
void as_initialize(void);

#endif
