#ifndef __ADDRSPACE_H
#define __ADDRSPACE_H

#include <chinos/config.h>

typedef struct mem_region
{
    unsigned long pbase;
    unsigned long vbase;
    unsigned long size;
} mem_region_t;

void as_initialize(void);

#endif
