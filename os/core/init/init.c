#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <k_assert.h>
#include <uapi/util.h>
#include <uart.h>
#include <board.h>
#include <irq.h>
#include <cpu.h>
#include <pagetable.h>
#include <addrspace.h>
#include <generic_timer.h>
#include <buddy.h>
#include <gran.h>
#include <idle.h>
#include <task.h>
#include <scheduler.h>
#include <elf_loader.h>
#include <elf.h>

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

    kprintf("physic address [0x%016x, 0x%016x] | %.3f%cB | %s\n", start, end, size, type, name);

}

static void clear_bss(void)
{
    extern unsigned long kernel_bss_start;
    extern unsigned long kernel_bss_end;

    void *bss = (unsigned long *)&kernel_bss_start;
    unsigned long size = (unsigned long)&kernel_bss_end - (unsigned long)&kernel_bss_start;
    k_memset(bss, 0, size);
}

struct mem_region user_elf_region;

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

    extern unsigned long user_start;
    extern unsigned long user_end;

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

    user_elf_region.vbase = (unsigned long)&user_start;
    user_elf_region.pbase = vbase_to_pbase((unsigned long)&user_start);
    user_elf_region.size = (unsigned long)&user_end - (unsigned long)&user_start;

    dump_image_info(MMIO_PBASE, MMIO_PBASE + MMIO_SIZE, "Device");
    dump_image_info(vbase_to_pbase((unsigned long)&boot_physic_start), vbase_to_pbase((unsigned long)&boot_end), "Boot");
    dump_image_info(vbase_to_pbase((unsigned long)&boot_end), vbase_to_pbase((unsigned long)&kernel_end), "Kernel Image");
    dump_image_info(vbase_to_pbase((unsigned long)&user_start), vbase_to_pbase((unsigned long)&user_end), "User ELF");
    dump_image_info(vbase_to_pbase((unsigned long)&user_end), RAM_SIZE  + vbase_to_pbase((unsigned long)&kernel_start), "Free RAM");
}

struct mm_gran *g_heap;
struct graninfo g_heapinfo;
struct mem_region kernel_heap_region;

void mem_test(void)
{
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
}

static void mm_initialize(void)
{
    //extern unsigned long kernel_end;
    extern unsigned long user_end;
    extern unsigned long kernel_start;

    unsigned long p_heapstart = (unsigned long)&user_end;
    unsigned long p_heapend = RAM_SIZE  + (unsigned long)&kernel_start;
    size_t heapsize = p_heapend - p_heapstart;

    kprintf("heapstart = %p, p_heapend = %p, heapsize = %p\n", p_heapstart, p_heapend, heapsize);
    g_heap = gran_initialize((void *)p_heapstart, heapsize, PAGE_SHIFT, PAGE_SHIFT);
    kprintf("g_heap = %p, heapstart = %p, ngranules = %d\n", g_heap, g_heap->heapstart, g_heap->ngranules);

    kernel_heap_region.vbase = p_heapstart;
    kernel_heap_region.pbase = vbase_to_pbase(p_heapstart);
    kernel_heap_region.size = heapsize;

    //mm_test();

    gran_dump(g_heap, &g_heapinfo);
    kprintf("total page = %d, free page = %d, mx free page = %d\n", g_heapinfo.ngranules, g_heapinfo.nfree, g_heapinfo.mxfree);
}

static struct tcb idle_task;
extern unsigned long idle_stack;
static void idle_task_initialize(void)
{
    task_create(&idle_task, \
                    (task_entry)idle, \
                    "idle", \
                    CONFIG_MAX_TASK_PRIORITY-1, \
                    (void *)&idle_stack, \
                    CONFIG_IDLE_TASK_STACKSIZE, \
                    TASK_TYPE_KERNEL, \
                    &kernel_addrspace);

    task_set_tgid(&idle_task, idle_task.tid);

    sched_init(&idle_task);
}

static struct tcb root_task;
static struct chin_elf user_elf;
static void boot_user_elf(struct tcb *task, struct chin_elf *elf)
{
    int ret;

    k_memset(elf, 0, sizeof(struct chin_elf));

    INIT_LIST_HEAD(&elf->link_head);
    INIT_LIST_HEAD(&elf->segs);

    extern unsigned long user_start;
    extern unsigned long user_end;

    elf->buffer = (uint8_t *)&user_start;
    elf->size = (uint64_t)&user_end - (uint64_t)&user_start;

    ret = elf_initialize(task, elf);
    if (ret < 0) {
        PANIC();
    }
}

#define USER_STACK_TOP    ((1UL << VA_BITS) - CONFIG_DEFAULT_TASK_STACKSIZE)
static void root_task_create(void)
{
    task_create(&root_task, \
                (task_entry)user_elf.e_entry, \
                "root task", \
                CONFIG_DEFAULT_TASK_PRIORITY, \
                (void *)USER_STACK_TOP, \
                CONFIG_DEFAULT_TASK_STACKSIZE, \
                TASK_TYPE_USER, \
                &user_addrspace);

    task_set_tgid(&root_task, root_task.tid);

    // 解析app.elf并加载到root_task
    boot_user_elf(&root_task, &user_elf);

    // 给用户任务映射设备内存
    struct mem_region dev_region;
    dev_region.pbase = UART_PBASE;
    dev_region.vbase = 0x0000000f00000000UL;
    dev_region.size = 0x1000;
    as_map(root_task.addrspace, &dev_region, 0, RAM_DEVICE);

    // 给用户任务映射stack
    void *stack = gran_alloc(g_heap, CONFIG_DEFAULT_TASK_STACKSIZE);
    struct mem_region stack_region;
    stack_region.pbase = vbase_to_pbase((unsigned long)stack);
    stack_region.vbase = USER_STACK_TOP;
    stack_region.size  = CONFIG_DEFAULT_TASK_STACKSIZE;
    as_map(root_task.addrspace, &stack_region, PF_R|PF_W, RAM_NORMAL);

    sched_attach(&root_task);
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

    // 地址空间初始化
    kprintf("as_initialize\n");
    as_initialize();

#if 0
    // 地址空间映射权限测试
    // Data Abort
    //extern unsigned long kernel_code_start;
    //u64 *data = &kernel_code_start;
    //kprintf("data = %p\n", *data);
    //*data = 0x12345678;

    // Instruction Abort
    typedef void (*func)(void);
    extern unsigned long kernel_rodata_start;
    func ins = &kernel_rodata_start;
    ins();
#endif

    asid_initialize();

    // 创建idle任务并初始化调度器
    idle_task_initialize();

    // 创建第一个用户进程
    root_task_create();

    // timer初始化
    kprintf("timer_init\n");
    timer_init();

    // 打开中断
    kprintf("enable global irq\n");
    arch_local_irq_enable();

    idle();
}
