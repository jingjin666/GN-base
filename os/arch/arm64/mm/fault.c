#include <k_stdio.h>
#include <k_stddef.h>
#include <k_string.h>
#include <k_debug.h>
#include <scheduler.h>
#include <mmap.h>

#include "fault.h"

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

