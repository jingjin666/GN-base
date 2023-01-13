#include <chinos/config.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <uapi/util.h>
#include <board.h>
#include <irq.h>
#include <cpu.h>
#include <tlb.h>
#include <instructionset.h>
#include <barrier.h>
#include <pagetable.h>
#include <gran.h>
#include <init.h>
#include <task.h>
#include <mmap.h>

#include "mmu-hyper.h"

#define S2_MEMATTR_TYPE_DEVICE  (0 << 2)
#define S2_DEVICE_nGnRnE        (0)
#define S2_DEVICE_nGnRE         (1)
#define S2_DEVICE_nGRE          (2)
#define S2_DEVICE_GRE           (3)
#define S2_MEMATTR_TYPE_RAM     (1 << 2)
#define S2_RAM_RESERVED         (0)
#define S2_RAM_INNER_NC         (1)
#define S2_RAM_INNER_WT_C       (2)
#define S2_RAM_INNER_WB_C       (3)

extern const struct mem_region kernel_dev_ram[];

struct addrspace hyper_kernel_addrspace;
struct addrspace hyper_user_addrspace;

static unsigned long pt_calloc(size_t size)
{
    void *v_page = gran_alloc(g_heap, size);
    k_memset(v_page, 0, size);
    return (unsigned long)v_page;
}

static int region_map(struct page_table *pgtable, struct mem_region *region, pgprot_t prot)
{
    unsigned long vaddr = region->vbase;
    unsigned long paddr = region->pbase;
    unsigned long size = region->size;
    unsigned long attr = pgprot_val(prot);

    return pt_map((pgd_t *)pgtable, vaddr, paddr, size, attr, pt_calloc);
}

static int hyper_kernel_as_image_mapping(struct image_region *image)
{
    int ret;

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map code  : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->code.vbase, image->code.pbase, image->code.size, pgprot_val(PAGE_HYP_ROX));
    ret = region_map(&hyper_kernel_addrspace.pg_table, &image->code, PAGE_HYP_ROX);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map rodata: vbase:%p, pbase:%p, size:%p, attr:%p\n", image->rodata.vbase, image->rodata.pbase, image->rodata.size, pgprot_val(PAGE_HYP_RO));
    ret = region_map(&hyper_kernel_addrspace.pg_table, &image->rodata, PAGE_HYP_RO);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map data  : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->data.vbase, image->data.pbase, image->data.size, pgprot_val(PAGE_HYP));
    ret = region_map(&hyper_kernel_addrspace.pg_table, &image->data, PAGE_HYP);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map bss   : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->bss.vbase, image->bss.pbase, image->bss.size, pgprot_val(PAGE_HYP));
    ret = region_map(&hyper_kernel_addrspace.pg_table, &image->bss, PAGE_HYP);

    return ret;
}

static int hyper_kernel_as_dev_mapping(struct mem_region *region)
{
    int ret;

    while (region->size != 0) {
        kprintf("-------------------------------------------------------------------------------------------------\n");
        kprintf("| region_map dev   : vbase:%p, pbase:%p, size:%p, attr:%p\n", region->vbase, region->pbase, region->size, pgprot_val(PAGE_HYP_DEVICE));
        ret = region_map(&hyper_kernel_addrspace.pg_table, region, PAGE_HYP_DEVICE);
        region++;
    }
    return ret;
}

static int hyper_kernel_as_ram_mapping(struct mem_region *region)
{
    int ret;

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map ram   : vbase:%p, pbase:%p, size:%p, attr:%p\n", region->vbase, region->pbase, region->size, pgprot_val(PAGE_HYP));
    ret = region_map(&hyper_kernel_addrspace.pg_table, region, PAGE_HYP);

    return ret;
}

static void hyper_kernel_as_switch(void)
{
    kprintf("hyper_kernel_addrspace = %p\n", &hyper_kernel_addrspace.pg_table);

    unsigned long ks_pbase = vbase_to_pbase((unsigned long)&hyper_kernel_addrspace.pg_table);

    /* TTBR0 */
    u64 ttbr0;
    MSR("TTBR0_EL2", ks_pbase);
    MRS("TTBR0_EL2", ttbr0);
    kprintf("TTBR0_EL2 = 0x%lx\n", ttbr0);

    // flush tlb
    flush_TLB();

    kprintf("switch to kernel addrspace ok\n");
}

void hyper_as_switch(struct addrspace *as, unsigned long type, asid_t asid)
{
    kprintf("switch to addrspace = %p, type = %p, asid = %p\n", &as->pg_table, type, asid);

    unsigned long pbase = vbase_to_pbase((unsigned long)&as->pg_table);
    if (type == TASK_TYPE_KERNEL) {
        // 除了idle外，没有其他内核进程，无需进行页表切换
        #if 0
        /* TTBR1 */
        u64 ttbr1;
        MSR("TTBR1_EL2", pbase);
        MRS("TTBR1_EL2", ttbr1);
        //kprintf("TTBR1_EL2 = %p\n", ttbr1);
        #endif
    } else if (type == TASK_TYPE_USER) {
        /* VTTBR_EL2 */
        u64 ttbr0;
        MRS("VTTBR_EL2", ttbr0);
        kprintf("prev:VTTBR_EL2 = %p\n", ttbr0);

        MSR("VTTBR_EL2", pbase | asid);

        MRS("VTTBR_EL2", ttbr0);
        kprintf("next:VTTBR_EL2 = %p\n", ttbr0);
    } else {
        PANIC();
    }

    kprintf("switch to addrspace ok\n");
}

void hyper_as_initialize(void)
{
    // 根据内核不同section属性来映射内核image
    hyper_kernel_as_image_mapping(&kernel_image);
    // 映射用户ELF
    hyper_kernel_as_ram_mapping(&user_elf_region);
    // 映射内核堆内存
    hyper_kernel_as_ram_mapping(&kernel_heap_region);
    // 映射内核设备内存
    hyper_kernel_as_dev_mapping((struct mem_region *)kernel_dev_ram);
    // 从恒等映射表切换到内核页表
    hyper_kernel_as_switch();
}

int hyper_as_map(struct addrspace *as, struct mem_region *region, uint32_t prot, RAM_TYPE_e type)
{
    unsigned long vaddr = region->vbase;
    unsigned long paddr = region->pbase;
    unsigned long size = region->size;
    unsigned long attr = _PROT_DEFAULT | PTE_HYP | PTE_WRITE | PTE_PXN | PTE_HYP_XN;

    // S2AP[1:0]
    if (prot & PROT_READ) {
       attr &= ~PTE_RDONLY;
    }

    // XN[1:0],Execute-never, stage 2 only
    if (prot & PROT_EXEC) {
        // The stage 2 control permits execution at EL1 and EL0
        attr &= ~PTE_HYP_XN;
        attr &= ~PTE_PXN;
    }

    // S2AP[1:0]
    if (prot & PROT_WRITE) {
        attr |= PTE_RDONLY;
    }

    // MemAttr[3:0]
    if (type == RAM_NORMAL) {
        // normal ram
        attr |= PTE_ATTRINDX(S2_MEMATTR_TYPE_RAM | S2_RAM_INNER_WB_C);
    } else if (type == RAM_DEVICE) {
        // device ram
        attr |= PTE_ATTRINDX(S2_MEMATTR_TYPE_DEVICE | S2_DEVICE_nGnRE);
    } else {
        // todo
        PANIC();
    }

    kprintf("hyper_as_map pbase:%p vbase:%p size:%p attr:%p\n", region->pbase, region->vbase, region->size, attr);
    return pt_map((pgd_t *)&as->pg_table, vaddr, paddr, size, attr, pt_calloc); 
}

#endif
