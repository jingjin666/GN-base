#include <chinos/config.h>

#include <k_stdio.h>
#include <k_assert.h>
#include <k_stdint.h>
#include <k_stdlib.h>
#include <k_string.h>
#include <uapi/util.h>

#ifdef CONFIG_MM_BUDDY
#include <mm_buddy.h>
#elif defined(CONFIG_MM_GRAN)
#include <gran.h>
#endif

#include <mm_heap.h>

#ifdef CONFIG_MM_BUDDY
buddy_t *g_heap;
#elif defined(CONFIG_MM_GRAN)
struct mm_gran *g_heap;
struct graninfo g_heapinfo;
#endif

struct mem_region kernel_heap_region;

static void *heap_alloc(size_t s)
{
#ifdef CONFIG_MM_BUDDY
    if (s <= CONFIG_PAGE_SIZE) {
        // allocate from slab
    }
    return buddy_alloc(g_heap, ALIGN(s, CONFIG_PAGE_SIZE) >> CONFIG_PAGE_SHIFT);
#elif defined(CONFIG_MM_GRAN)
    return gran_alloc(g_heap, s);
#endif
}

static void heap_free(void *p, size_t s)
{
#ifdef CONFIG_MM_BUDDY
    return buddy_free(g_heap, p);
#elif defined(CONFIG_MM_GRAN)
    // gran free need parameter of memory size 
    // 不能做到kfree/heap_free级别的接口兼容
    PANIC();
    gran_free(g_heap, p, s);
#endif
}

static void mm_test(void)
{
    void *addr1 ;
    void *addr2 ;
    void *addr3 ;
    void *addr4 ;

    addr1 = heap_alloc(1024);
    kprintf("addr1 = %p\n", addr1);
    addr2 = heap_alloc(1024);
    kprintf("addr2 = %p\n", addr2);
    addr3 = heap_alloc(1024);
    kprintf("addr3 = %p\n", addr3);
    addr4 = heap_alloc(1024);
    kprintf("addr4 = %p\n", addr4);

    heap_free(addr1, 1024);
    heap_free(addr4, 1024);

    addr1 = heap_alloc(4*1024*1024);
    kprintf("addr1 = %p\n", addr1);

    addr1 = heap_alloc(1024*1024);
    kprintf("addr1 = %p\n", addr1);

    while(1);
}

void mm_initialize(void)
{
    //extern unsigned long kernel_end;
    extern unsigned long user_end;
    extern unsigned long kernel_start;

    unsigned long p_heapstart = (unsigned long)&user_end;
    unsigned long p_heapend = RAM_SIZE  + (unsigned long)&kernel_start - RAM_OFFSET;
    size_t heapsize = p_heapend - p_heapstart;

    kprintf("heapstart = %p, p_heapend = %p, heapsize = %p\n", p_heapstart, p_heapend, heapsize);

    kernel_heap_region.vbase = p_heapstart;
    kernel_heap_region.pbase = vbase_to_pbase(p_heapstart);
    kernel_heap_region.size = heapsize;

#ifdef CONFIG_MM_BUDDY
    g_heap = buddy_init((void *)p_heapstart, heapsize >> PAGE_SHIFT, 1);
#elif defined(CONFIG_MM_GRAN)
    g_heap = gran_initialize((void *)p_heapstart, heapsize, PAGE_SHIFT, PAGE_SHIFT);
    kprintf("g_heap = %p, heapstart = %p, ngranules = %d\n", g_heap, g_heap->heapstart, g_heap->ngranules);

    gran_dump(g_heap, &g_heapinfo);
    kprintf("total page = %d, free page = %d, mx free page = %d\n", g_heapinfo.ngranules, g_heapinfo.nfree, g_heapinfo.mxfree);
#endif

    //mm_test();
}

void *kmalloc(size_t s)
{
    return heap_alloc(s);
}

void *kcalloc(size_t n, size_t s)
{
	if (s && n > (size_t)-1/s) {
		return 0;
	}

    size_t size = n * s;

    void *p = kmalloc(size);

    if (p) {
        k_memset(p, 0, size);
    }

    return p;
}

void kfree(void *p)
{
    return heap_free(p, 0);
}
