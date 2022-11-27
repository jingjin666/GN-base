#include <chinos/config.h>

#include <k_stdio.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_string.h>
#include <k_debug.h>
#include <scheduler.h>
#include <task.h>

#include "mmap.h"

#define MMAP_DEBUG
#ifdef MMAP_DEBUG
#define mmap_dbg kdbg
#define mmap_err kerr
#define mmap_warn kwarn
#define mmap_info kinfo
#else
#define mmap_dbg(fmt, ...)
#define mmap_err(fmt, ...)
#define mmap_warn(fmt, ...)
#define mmap_info(fmt, ...)
#endif

#include <init.h>
#include <gran.h>
static void *mmap_calloc(size_t size)
{
    void *p = gran_alloc(g_heap, size);
    k_memset(p, 0, size);
    mmap_dbg("mmap_calloc p = %p\n", p);
    return p;
}

unsigned long sys_brk(unsigned long brk)
{
	struct tcb *current = this_task();
	struct mm_area *mm = &current->mm;

    kprintf("sys_brk(%p)\n", brk);
    if (brk < mm->start_brk) {
        goto out;
    }

    mm->brk = brk;

out:
    kprintf("brk = %p\n", mm->brk);
    return mm->brk;
}

void sys_dumpvma(void)
{
	tcb_t *current = this_task();
	struct mm_area *mm = &current->mm;

    kprintf("------------------------------------------\n");

	kprintf("0x%08x - 0x%08x r-x    %s\n", mm->start_code, mm->end_code, current->name);
	kprintf("0x%08x - 0x%08x rw-    %s\n", mm->start_data, mm->end_data, current->name);

	kprintf("0x%08x - 0x%08x rw-    %s\n", mm->start_brk, mm->brk, "[heap]");

	struct vm_area *vma;
	list_for_each_entry(vma, &mm->link_head, link_head)
	{
		kprintf("%p - %p %c%c%c    [%s]\n", vma->vm_start, vma->vm_end, 
			vma->vm_prot & PROT_READ  ? 'r' : '-', 
			vma->vm_prot & PROT_WRITE ? 'w' : '-', 
			vma->vm_prot & PROT_EXEC  ? 'x' : '-', 
			vma->vm_name);
	}

	kprintf("%p - %p rw-    %s\n", mm->start_stack, mm->end_stack, "[stack]");

    kprintf("------------------------------------------\n");
}

void find_free_vma(struct mm_area *mm, struct vm_area *vma, size_t len)
{
	struct vm_area *_vma;

    if (list_empty(&mm->link_head)) {
        vma->vm_start = mm->mmap_base - len;
        vma->vm_end = mm->mmap_base;
    }
    
	list_for_each_entry(_vma, &mm->link_head, link_head)
	{
        vma->vm_end = _vma->vm_start;
        vma->vm_start = _vma->vm_start - len;
        break;
	}
}

unsigned long sys_mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    if (flags & MAP_PRIVATE) {
        // 私有映射
        // 对映射区域得写入数据会产生映射文件的复制，即私人得“写时复写”对此区域得任何修改都不会写回到原文件
        if (flags & MAP_ANONYMOUS) {
            // 私有匿名映射
            // 建立匿名映射会忽略参数fd，不涉及文件，而且映射的区域无法和其他进程共享
            if (fd == -1) {
                // 当使用参数 fd=-1 且 flags=MAP_ANONYMOUS | MAP_PRIVATE 时，创建的 mmap 映射是私有匿名映射
                // 私有匿名映射最常见的用途是在 glibc 分配大块内存中，当需要的分配的内存大于 MMAP_THREASHOLD(128KB) 时，glibc会默认使用 mmap 代替 brk 来分配内存
                mmap_dbg("MAP_PRIVATE | MAP_ANONYMOUS\n");
            }
        } else {
            // 私有文件映射
            // 私有文件映射时 flags 的标志位被设置为 MAP_PRIVATE，那么就会创建私有文件映射
            // 私有文件映射的最常用的场景是加载动态共享库
            mmap_dbg("MAP_PRIVATE\n");
        }
    } else if (flags & MAP_SHARED) {
        // 共享映射
        // 对映射区域得写入数据会复制回写到原文件内，而且允许其他映射该文件得进程共享
        if (flags & MAP_ANONYMOUS) {
            // 共享匿名映射
            // 建立匿名映射会忽略参数fd，不涉及文件，而且映射的区域无法和其他进程共享
            if (fd == -1) {
                // 当使用参数 fd=-1 且 flags=MAP_ANONYMOUS | MAP_SHARED 时，创建的 mmap 映射是共享匿名映射
                // 共享匿名映射让相关进程共享一块内存区域，通常用于父子进程的之间通信
                mmap_dbg("MAP_SHARED | MAP_ANONYMOUS\n");
            }
        } else {
            // 共享文件映射
            // 创建文件映射时 flags 的标志位被设置为 MAP_SHARED，那么就会创建共享文件映射
            // 共享文件映射通常有如下场景：
            // 读写文件：把文件内容映射到进程地址空间，同时对映射的内容做了修改，内核的回写机制（writeback）最终会把修改的内容同步到磁盘中
            // 进程间通信：
            // 进程之间的进程地址空间相互隔离，一个进程不能访问到另外一个进程的地址空间。如果多个进程都同时映射到一个相同的文件，就实现了多进程间的共享内存的通信。如果一个进程对映射内容做了修改，那么另外的进程是可以看到的
            mmap_dbg("MAP_SHARED\n");
        }
    } else {
        mmap_err("Unsupport mmap flags");
        return MAP_FAILED;
    }

    if (len == 0) {
        return MAP_FAILED;
    }

    struct tcb *current = this_task();
    struct mm_area *mm = &current->mm;
    struct mem_region region;

    sys_dumpvma();

    struct vm_area *vma = (struct vm_area *)mmap_calloc(sizeof(struct vm_area));
    INIT_LIST_HEAD(&vma->link_head);
    vma->vm_mm = mm;
    vma->vm_prot = prot;
    vma->vm_flags = flags;
    if (flags == (MAP_PRIVATE))
        k_strcpy(vma->vm_name, "MAP_PRIVATE");
    else if (flags == (MAP_PRIVATE | MAP_ANONYMOUS))
        k_strcpy(vma->vm_name, "MAP_PRIVATE | MAP_ANONYMOUS");
    else if (flags == (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED))
        k_strcpy(vma->vm_name, "MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED");
    else if (flags == (MAP_SHARED))
        k_strcpy(vma->vm_name, "MAP_SHARED");
    else if (flags == (MAP_SHARED | MAP_ANONYMOUS))
        k_strcpy(vma->vm_name, "MAP_SHARED | MAP_ANONYMOUS");
    else if (flags == (MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED))
        k_strcpy(vma->vm_name, "MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED");
    else
        k_strcpy(vma->vm_name, "UNKNOWN");

    mmap_dbg("start = %p\n", start);
    if (start == NULL) {
        // 寻找合适的一块VM_AREA
        find_free_vma(mm, vma, len);
        start = vma->vm_start;
    } else {
        vma->vm_start = start;
        vma->vm_end = start + len;
    }
    mmap_dbg("start = %p\n", start);

    list_add(&vma->link_head, &mm->link_head);

    sys_dumpvma();

    unsigned long vaddr = (unsigned long)mmap_calloc(len);

    region.pbase = vbase_to_pbase(vaddr);
    region.vbase = start;
    region.size  = len;

    as_map(current->addrspace, &region, prot, RAM_NORMAL);

    dump_pgtable_verbose(&current->addrspace->pg_table, 0);

    return start;
}
