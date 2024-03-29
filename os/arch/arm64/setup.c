#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <uart.h>
#include <instructionset.h>
#include <page-def.h>
#include <pgtable-prot.h>
#include <kernel-pgtable.h>
#include <system.h>
#include <barrier.h>
#include <pagetable.h>
#include <addrspace.h>
#include <uapi/util.h>
#include <board.h>

// 存放恒等映射和初始化映射的页表,这一阶段都使用段式映射,页表空间使用较小,这里初始化4M空间来进行存储管理
#define EARLY_PGTABLE_NR_PAGES  1024
#define EARLY_PGTABLE_MM_SIZE  (PAGE_SIZE * EARLY_PGTABLE_NR_PAGES)

/* Kernel内存空间映射 */
const struct mem_region kernel_normal_ram[] = {
    {
        .pbase = RAM_PBASE,
        .vbase = RAM_VBASE,
        .size = RAM_SIZE,
    },
    {.size = 0}, // end of regions
};

/* Kernel设备空间映射*/
const struct mem_region kernel_dev_ram[] = {
    // MMIO
    {
        .pbase = MMIO_PBASE,
        .vbase = MMIO_VBASE,
        .size =  MMIO_SIZE
    },
    {.size = 0}, // end of regions
};

/*
 * 为什么要使用两个页表来进行启动映射
 * 在非虚拟化场景下，恒等映射和内核映射的起始虚拟地址对应的idx相同，这样会导致内核映射会覆盖掉恒等映射，导致内核无法启动
 * 在虚拟化场景下（NVHE），恒等映射的地址和内核映射的地址对应的idx不同，这样就不会存在映射覆盖问题，但这也是一个不太合理的操作
 * 在虚拟化场景下（VHE），由于存在TTBR1_EL2，则可以兼容非虚拟化和虚拟化这两种场景下的映射，通过使用两个启动页表来进行内核启动，这样同时解决了VNHE下的那个不合理操作的问题
 */
// 恒等映射的PGD页表
static BOOTDATA struct page_table identifymap_pt;
// 内核启动的PGD页表
static BOOTDATA struct page_table bootkernelmap_pt;

// 页表空间
static BOOTDATA char idmap_pt_space[EARLY_PGTABLE_MM_SIZE];
// 页表内存管理的数据结构
static BOOTDATA char idmap_pt_mm[EARLY_PGTABLE_NR_PAGES];

static void BOOTPHYSIC boot_pt_init(void)
{
    k_memset(&identifymap_pt, 0, sizeof(identifymap_pt));
    k_memset(&bootkernelmap_pt, 0, sizeof(bootkernelmap_pt));
    k_memset(idmap_pt_space, 0, sizeof(idmap_pt_space));
    k_memset(idmap_pt_mm, 0, sizeof(idmap_pt_mm));
}

static u64 BOOTPHYSIC idmap_pt_info(void)
{
   u64 free = 0;
   for (int i = 0; i < EARLY_PGTABLE_NR_PAGES; i++) {
        if (idmap_pt_mm[i] == 0) {
            free++;
        }
   }
   return free;
}

u64 BOOTPHYSIC early_pgtable_alloc(u64 size)
{
    for (int i = 0; i < EARLY_PGTABLE_NR_PAGES; i++) {
        if (idmap_pt_mm[i] == 0) {
            idmap_pt_mm[i] = 1;
            return (u64)&idmap_pt_space[i * PAGE_SIZE];
        }
    }
    return 0;
}

static void BOOTPHYSIC boot_identify_mapping(void)
{
    // 注意此刻内存环境都在物理地址空间下进行
    extern unsigned long boot_physic_start;
    extern unsigned long boot_end;
    u64 vaddr, paddr, size, attr;

    vaddr = (u64)&boot_physic_start;
    paddr = (u64)&boot_physic_start;
    size = ALIGN((u64)&boot_end - (u64)&boot_physic_start, SECTION_SIZE);
    attr = pgprot_val(PAGE_KERNEL_EXEC);
    kprintf("%s:%d#pg_map: va:%p, pa:%p, siza:%p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    idmap_pt_map((pgd_t *)&identifymap_pt, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pt, 1);
}

static void BOOTPHYSIC boot_kernel_mapping(void)
{

    //extern unsigned long kernel_start;
    //extern unsigned long kernel_end;
    u64 vaddr, paddr, size, attr;

    // 映射内核普通内存
    vaddr = kernel_normal_ram[0].vbase;
    paddr = kernel_normal_ram[0].pbase;
    size = ALIGN(kernel_normal_ram[0].size, SECTION_SIZE);
    attr = pgprot_val(PAGE_KERNEL_EXEC);
    kprintf("%s:%d#pg_map: va:%p, pa:%p, size:%p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    idmap_pt_map((pgd_t *)&bootkernelmap_pt, vaddr, paddr, size, attr, early_pgtable_alloc);
    //dump_pgtable_verbose((pgd_t *)&bootkernelmap_pt, 1);

    // 映射内核设备内存
    vaddr = kernel_dev_ram[0].vbase;
    paddr = kernel_dev_ram[0].pbase;
    size = ALIGN(kernel_dev_ram[0].size, SECTION_SIZE);
    attr = PROT_SECT_DEVICE_nGnRE;
    kprintf("%s:%d#pg_map: va:%p, pa:%p, size:%p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    idmap_pt_map((pgd_t *)&bootkernelmap_pt, vaddr, paddr, size, attr, early_pgtable_alloc);
    //dump_pgtable_verbose((pgd_t *)&bootkernelmap_pt, 1);

    /* 注意此处需要使能TTBR0，打开MMU后，在进入虚拟地址空间前，任然还有一些代码运行于物理内存中 */
    /* TTBR0 */
    u64 ttbr0;
    MSR("TTBR0_EL1", (u64)&identifymap_pt);
    MRS("TTBR0_EL1", ttbr0);
    kprintf("TTBR0_EL1 = 0x%lx\n", ttbr0);

    /* TTBR1 */
    u64 ttbr1;
    MSR("TTBR1_EL1", (u64)&bootkernelmap_pt);
    MRS("TTBR1_EL1", ttbr1);
    kprintf("TTBR1_EL1 = 0x%lx\n", ttbr1);
}

void BOOTPHYSIC boot_setup_mmu(void)
{
    uart_init(UART_PBASE);

    u64 current_el;
    MRS("CurrentEL", current_el);
    current_el = bitfield_get(current_el, 2, 2);
    kprintf("CurrentEL = EL%d\n", current_el);

    /* 内存属性标识 */
    u64 mair;
    MSR("MAIR_EL1", MAIR_EL1_SET);
    MRS("MAIR_EL1", mair);
    kprintf("MAIR_EL1 = 0x%lx\n", mair);

    /* 当前Core支持的物理地址范围 */
    u64 mmfr0, pa_range, asid_range;
    MRS("ID_AA64MMFR0_EL1", mmfr0);
    pa_range = bitfield_get(mmfr0, ID_AA64MMFR0_PARANGE_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.PARANGE = 0x%lx\n", pa_range);

    asid_range = bitfield_get(mmfr0, ID_AA64MMFR0_ASID_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.ASIDBITS = 0x%lx\n", asid_range);

    u64 tcr;
    MRS("TCR_EL1", tcr);
    kprintf("TCR_EL1 = 0x%lx\n", tcr);

    MSR("TCR_EL1", (pa_range << TCR_IPS_SHIFT) | \
                   TCR_T0SZ(VA_BITS) | \
                   TCR_T1SZ(VA_BITS) | \
                   TCR_TG0_4K | \
                   TCR_TG1_4K | \
                   TCR_SH0_INNER | \
                   TCR_SH1_INNER | \
                   TCR_ORGN0_WBWA | \
                   TCR_ORGN1_WBWA | \
                   TCR_IRGN0_WBWA | \
                   TCR_IRGN1_WBWA);

    MRS("TCR_EL1", tcr);
    kprintf("TCR_EL1 = 0x%lx\n", tcr);

    boot_pt_init();

    boot_identify_mapping();

    boot_kernel_mapping();

    u64 free_pg = idmap_pt_info();
    kprintf("early identify pgtable total pages = %d, free = %d\n", EARLY_PGTABLE_NR_PAGES, free_pg);
}

