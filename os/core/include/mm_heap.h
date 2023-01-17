#ifndef __MM_HEAP_H
#define __MM_HEAP_H

#include <chinos/config.h>

#include <k_types.h>
#include <addrspace.h>

#ifdef CONFIG_MM_BUDDY
#include <mm_buddy.h>
#elif defined(CONFIG_MM_GRAN)
#include <gran.h>

#endif

#ifdef CONFIG_MM_BUDDY

#elif defined(CONFIG_MM_GRAN)
extern struct mm_gran *g_heap;
extern struct graninfo g_heapinfo;
#endif

extern struct mem_region kernel_heap_region;

void mm_initialize(void);

#endif
