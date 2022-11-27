#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_string.h>
#include <k_debug.h>
#include <scheduler.h>
#include <mmap.h>

#include "fault.h"

#include <init.h>
#include <gran.h>
static void *fault_calloc(size_t size)
{
    void *p = gran_alloc(g_heap, size);
    k_memset(p, 0, size);
    kprintf("fault_calloc p = %p\n", p);
    return p;
}

void do_page_fault(unsigned long addr)
{
    struct tcb *current = this_task();
    struct mm_area *mm = &current->mm;
    struct mem_region region;
    
	if(addr >= mm->start_brk && addr < mm->brk)
	{
        // COW
        int heap_size = mm->brk - mm->start_brk;
        if (heap_size > 0) {
            unsigned long vaddr = (unsigned long)fault_calloc(heap_size);

            region.pbase = vbase_to_pbase(vaddr);
            region.vbase = mm->start_brk;
            region.size  = heap_size;
            as_map(current->addrspace, &region, PROT_READ|PROT_WRITE, RAM_NORMAL);
            dump_pgtable_verbose(&current->addrspace->pg_table, 0);
        }

        schedule();
        restore_current_context();
    }
}

