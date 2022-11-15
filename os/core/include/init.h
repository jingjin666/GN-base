#ifndef __INIT_H
#define __INIT_H

#include <kernel.h>
#include <addrspace.h>

typedef struct image_region {
    struct mem_region code;
    struct mem_region rodata;
    struct mem_region data;
    struct mem_region bss;
} image_region_t;

extern struct image_region kernel_image;
extern struct mm_gran *g_heap;
extern struct graninfo g_heapinfo;
extern struct mem_region kernel_heap_region;

void init_kernel(void);
#endif
