#include <chinos/config.h>

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
#include <elf.h>

#include "addrspace.h"

extern const struct mem_region kernel_dev_ram[];

struct addrspace kernel_addrspace;

static unsigned long pg_alloc(size_t size)
{
    void *v_page = gran_alloc(g_heap, size);
    unsigned long p_page = vbase_to_pbase((unsigned long)v_page);
    kprintf("p_page = %p\n", p_page);
    return p_page;
}

static int region_map(struct page_table *pgtable, struct mem_region *region, pgprot_t prot)
{
    unsigned long vaddr = region->vbase;
    unsigned long paddr = region->pbase;
    unsigned long size = region->size;
    unsigned long attr = pgprot_val(prot);

    return pg_map((pgd_t *)pgtable, vaddr, paddr, size, attr, pg_alloc);
}

static int kernel_as_image_mapping(struct image_region *image)
{
    int ret;

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map code  : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->code.vbase, image->code.pbase, image->code.size, pgprot_val(PAGE_KERNEL_ROX));
    ret = region_map(&kernel_addrspace.pg_table, &image->code, PAGE_KERNEL_ROX);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map rodata: vbase:%p, pbase:%p, size:%p, attr:%p\n", image->rodata.vbase, image->rodata.pbase, image->rodata.size, pgprot_val(PAGE_KERNEL_RO));
    ret = region_map(&kernel_addrspace.pg_table, &image->rodata, PAGE_KERNEL_RO);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map data  : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->data.vbase, image->data.pbase, image->data.size, pgprot_val(PAGE_KERNEL));
    ret = region_map(&kernel_addrspace.pg_table, &image->data, PAGE_KERNEL);

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map bss   : vbase:%p, pbase:%p, size:%p, attr:%p\n", image->bss.vbase, image->bss.pbase, image->bss.size, pgprot_val(PAGE_KERNEL));
    ret = region_map(&kernel_addrspace.pg_table, &image->bss, PAGE_KERNEL);

    return ret;
}

static int kernel_as_dev_mapping(struct mem_region *region)
{
    int ret;

    while (region->size != 0) {
        kprintf("-------------------------------------------------------------------------------------------------\n");
        kprintf("| region_map dev   : vbase:%p, pbase:%p, size:%p, attr:%p\n", region->vbase, region->pbase, region->size, PROT_SECT_DEVICE_nGnRE);
        ret = region_map(&kernel_addrspace.pg_table, region, __pgprot(PROT_SECT_DEVICE_nGnRE));
        region++;
    }
    return ret;
}

static int kernel_as_ram_mapping(struct mem_region *region)
{
    int ret;

    kprintf("-------------------------------------------------------------------------------------------------\n");
    kprintf("| region_map ram   : vbase:%p, pbase:%p, size:%p, attr:%p\n", region->vbase, region->pbase, region->size, pgprot_val(PAGE_KERNEL));
    ret = region_map(&kernel_addrspace.pg_table, region, PAGE_KERNEL);

    return ret;
}

static void kernel_as_switch(void)
{
    kprintf("kernel_addrspace = %p\n", &kernel_addrspace.pg_table);

    unsigned long ks_pbase = vbase_to_pbase((unsigned long)&kernel_addrspace.pg_table);

    /* TTBR0 */
    u64 ttbr0;
    MSR("TTBR0_EL1", 0/*ks_pbase*/);
    MRS("TTBR0_EL1", ttbr0);
    kprintf("TTBR0_EL1 = 0x%lx\n", ttbr0);

    /* TTBR1 */
    u64 ttbr1;

    MSR("TTBR1_EL1", ks_pbase);
    MRS("TTBR1_EL1", ttbr1);
    kprintf("TTBR1_EL1 = 0x%lx\n", ttbr1);

    // flush tlb
    flush_TLB();

    kprintf("switch to kernel addrspace ok\n");
}

void as_switch(struct addrspace *as)
{

}

void as_initialize(void)
{
    // 根据内核不同section属性来映射内核image
    kernel_as_image_mapping(&kernel_image);
    // 映射用户ELF
    kernel_as_ram_mapping(&user_elf_region);
    // 映射内核堆内存
    kernel_as_ram_mapping(&kernel_heap_region);
    // 映射内核设备内存
    kernel_as_dev_mapping((struct mem_region *)kernel_dev_ram);
    // 从恒等映射表切换到内核页表
    kernel_as_switch();
}

static unsigned long _pg_alloc(size_t size)
{
    void *v_page = gran_alloc(g_heap, size);
    //unsigned long p_page = vbase_to_pbase((unsigned long)v_page);
    //kprintf("p_page = %p\n", p_page);
    //return p_page;
    kprintf("v_page = %p\n", v_page);
    return (unsigned long)v_page;
}

int as_map(struct addrspace *as, struct mem_region *region, uint32_t flag)
{
    unsigned long vaddr = region->vbase;
    unsigned long paddr = region->pbase;
    unsigned long size = region->size;
    unsigned long attr;

    if (flag == (PF_R|PF_X)) {
        attr = pgprot_val(PAGE_KERNEL_ROX);
    } else if (flag == (PF_R|PF_W)) {
        attr = pgprot_val(PAGE_KERNEL);
    } else {
        // todo
        PANIC();
    }

    return pg_map((pgd_t *)&as->pg_table, vaddr, paddr, size, attr, _pg_alloc);
}
