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
    
	if(addr >= mm->start_brk && addr < mm->brk)
	{
	    // No COW
        schedule();
        restore_current_context();
    }
}

