#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <uapi/util.h>
#include <uart.h>
#include <board.h>
#include <irq.h>
#include <cpu.h>
#include <pagetable.h>
#include <generic_timer.h>
#include <buddy.h>
#include <gran.h>

#include "init.h"

struct image_region kernel_image;

static void dump_image_info(unsigned long start, unsigned long end, const char *name)
{
    s64 length;
    float size;
    char type;

    length = end - start;
    if (length >= 0 && length < 1024) {
        type = ' ';
        size = length;
    } else if (length >= 1024 && length < 1024*1024) {
        type = 'K';
        size = length / 1024.f;
    } else if (length >= 1024*1024 && length < 1024*1024*1024UL) {
        type = 'M';
        size = length / 1024 / 1024.f;
    } else if (length >= 1024*1024*1024UL && length < 1024*1024*1024*1024UL) {
        type = 'G';
        size = length / 1024 / 1024 / 1024.f;
    } else {
        kprintf("invalid image sectoin information\n");
    }

    kprintf("physic address [%p, %p] | %.3f%cB | %s\n", start, end, size, type, name);

}

static void clear_bss(void)
{
    extern unsigned long kernel_bss_start;
    extern unsigned long kernel_bss_end;

    void *bss = (unsigned long *)&kernel_bss_start;
    unsigned long size = (unsigned long)&kernel_bss_end - (unsigned long)&kernel_bss_start;
    k_memset(bss, 0, size);
}

void init_image_info(void)
{
    extern unsigned long kernel_start;

    extern unsigned long boot_physic_start;
    extern unsigned long boot_end;

    extern unsigned long kernel_code_start;
    extern unsigned long kernel_code_end;

    extern unsigned long kernel_rodata_start;
    extern unsigned long kernel_rodata_end;

    extern unsigned long kernel_data_start;
    extern unsigned long kernel_data_end;

    extern unsigned long kernel_bss_start;
    extern unsigned long kernel_bss_end;

    extern unsigned long kernel_end;

    kernel_image.code.vbase = (unsigned long)&kernel_code_start;
    kernel_image.code.pbase = vbase_to_pbase((unsigned long)&kernel_code_start);
    kernel_image.code.size = (unsigned long)&kernel_code_end - (unsigned long)&kernel_code_start;

    kernel_image.rodata.vbase = (unsigned long)&kernel_rodata_start;
    kernel_image.rodata.pbase = vbase_to_pbase((unsigned long)&kernel_rodata_start);
    kernel_image.rodata.size = (unsigned long)&kernel_rodata_end - (unsigned long)&kernel_rodata_start;

    kernel_image.data.vbase = (unsigned long)&kernel_data_start;
    kernel_image.data.pbase = vbase_to_pbase((unsigned long)&kernel_data_start);
    kernel_image.data.size = (unsigned long)&kernel_data_end - (unsigned long)&kernel_data_start;

    kernel_image.bss.vbase = (unsigned long)&kernel_bss_start;
    kernel_image.bss.pbase = vbase_to_pbase((unsigned long)&kernel_bss_start);
    kernel_image.bss.size = (unsigned long)&kernel_bss_end - (unsigned long)&kernel_bss_start;

    dump_image_info(MMIO_PBASE, MMIO_PBASE + MMIO_SIZE, "Device");
    dump_image_info(vbase_to_pbase((unsigned long)&boot_physic_start), vbase_to_pbase((unsigned long)&boot_end), "Boot");
    dump_image_info(vbase_to_pbase((unsigned long)&boot_end), vbase_to_pbase((unsigned long)&kernel_end), "Kernel Image");
    dump_image_info(vbase_to_pbase((unsigned long)&kernel_end), RAM_SIZE  + vbase_to_pbase((unsigned long)&kernel_start), "Free RAM");
}

struct mm_gran *g_heap;
struct graninfo g_heapinfo;

static void mm_initialize(void)
{
    extern unsigned long kernel_end;
    extern unsigned long kernel_start;

    unsigned long p_heapstart = (unsigned long)&kernel_end;
    unsigned long p_heapend = RAM_SIZE  + (unsigned long)&kernel_start;
    size_t heapsize = p_heapend - p_heapstart;

    kprintf("heapstart = %p, p_heapend = %p, heapsize = %p\n", p_heapstart, p_heapend, heapsize);
    g_heap = gran_initialize((void *)p_heapstart, heapsize, PAGE_SHIFT, PAGE_SHIFT);
    kprintf("g_heap = %p, heapstart = %p, ngranules = %d\n", g_heap, g_heap->heapstart, g_heap->ngranules);

#if 0
    // mm_gran test
    void *addr1 ;
    void *addr2 ;
    void *addr3 ;
    void *addr4 ;

    addr1 = gran_alloc(g_heap, 1024);
    kprintf("addr1 = %p\n", addr1);
    addr2 = gran_alloc(g_heap, 1024);
    kprintf("addr2 = %p\n", addr2);
    addr3 = gran_alloc(g_heap, 1024);
    kprintf("addr3 = %p\n", addr3);
    addr4 = gran_alloc(g_heap, 1024);
    kprintf("addr4 = %p\n", addr4);

    gran_free(g_heap, addr1, 1024);
    gran_free(g_heap, addr4, 1024);
#endif

    gran_dump(g_heap, &g_heapinfo);
    kprintf("total page = %d, free page = %d, mx free page = %d\n", g_heapinfo.ngranules, g_heapinfo.nfree, g_heapinfo.mxfree);
}

void init_kernel(void)
{
    // 必须最先执行
    clear_bss();

    // 串口初始化
    uart_init(UART_VBASE);

    // 现在可以开始使用串口
    kprintf("kernel init...\n");

    // 填充内核映像
    init_image_info();

    // cpu初始化
    cpu_init();

    // 物理内存管理初始化
    mm_initialize();

    // 中断初始化
    kprintf("irq_initialize\n");
    irq_initialize();

    // timer初始化
    kprintf("timer_init\n");
    timer_init();

    // 打开中断
    arch_local_irq_enable();
}
