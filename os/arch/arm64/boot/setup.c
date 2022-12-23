#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <k_assert.h>
#include <uart.h>
#include <instructionset.h>
#include <page-def.h>
#include <pgtable-prot.h>
#include <kernel-pgtable.h>
#include <sysreg.h>
#include <barrier.h>
#include <pagetable.h>
#include <addrspace.h>
#include <uapi/util.h>
#include <board.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT
#include <vcpu.h>
#include <hyper.h>
#endif

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

// 恒等映射的PGD页表
static BOOTDATA struct page_table identifymap_pt;
// 页表空间
static BOOTDATA char idmap_pt_space[EARLY_PGTABLE_MM_SIZE];
// 页表内存管理的数据结构
static BOOTDATA char idmap_pt_mm[EARLY_PGTABLE_NR_PAGES];

static void BOOTPHYSIC idmap_pt_init(void)
{
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
    //extern unsigned long boot_end;
    u64 vaddr, paddr, size, attr;

    vaddr = (u64)&boot_physic_start;
    paddr = (u64)&boot_physic_start;
    size = kernel_normal_ram[0].size;//(u64)&boot_end - (u64)&boot_physic_start;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    attr = pgprot_val(PAGE_HYP_RWX);
#else
    attr = pgprot_val(PAGE_KERNEL_RWX);
#endif
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
    size = kernel_normal_ram[0].size;//(u64)&kernel_end - (u64)&kernel_start;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    attr = pgprot_val(PAGE_HYP_RWX);
#else
    attr = pgprot_val(PAGE_KERNEL_RWX);
#endif
    kprintf("%s:%d#pg_map: va:%p, pa:%p, size:%p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    idmap_pt_map((pgd_t *)&identifymap_pt, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pt, 1);

    // 映射内核设备内存
    vaddr = kernel_dev_ram[0].vbase;
    paddr = kernel_dev_ram[0].pbase;
    size = kernel_dev_ram[0].size;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    attr = pgprot_val(PAGE_HYP_DEVICE);
#else
    attr = PROT_SECT_DEVICE_nGnRE;
#endif
    kprintf("%s:%d#pg_map: va:%p, pa:%p, size:%p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    idmap_pt_map((pgd_t *)&identifymap_pt, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pt, 1);
}

static void BOOTPHYSIC set_ttbr(void)
{
    /* 注意此处需要使能TTBR0，打开MMU后，在进入虚拟地址空间前，任然还有一些代码运行于物理内存中 */

    u64 ttbr0;
    u64 ttbr1;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    /* TTBR0 */
    write_sysreg((u64)&identifymap_pt, TTBR0_EL2);
    ttbr0 = read_sysreg(TTBR0_EL2);
    kprintf("TTBR0_EL2 = 0x%lx\n", ttbr0);

    if (has_vhe()) {
        // VHE
        /* TTBR1 */
        kprintf("VHE\n");
        write_sysreg((u64)&identifymap_pt, TTBR1_EL2);
        ttbr1 = read_sysreg(TTBR1_EL2);
        kprintf("TTBR1_EL2 = 0x%lx\n", ttbr1);
    } else {
        // NVHE
        kprintf("NVHE\n");
    }
#else
    /* TTBR0 */
    MSR("TTBR0_EL1", (u64)&identifymap_pt);
    MRS("TTBR0_EL1", ttbr0);
    kprintf("TTBR0_EL1 = 0x%lx\n", ttbr0);

    /* TTBR1 */
    MSR("TTBR1_EL1", (u64)&identifymap_pt);
    MRS("TTBR1_EL1", ttbr1);
    kprintf("TTBR1_EL1 = 0x%lx\n", ttbr1);
#endif
}

void BOOTPHYSIC boot_setup_mmu(void)
{
    uart_init(UART_PBASE);

    u64 current_el;
    MRS("CurrentEL", current_el);
    current_el = bitfield_get(current_el, 2, 2);
    kprintf("CurrentEL = EL%d\n", current_el);

#ifdef CONFIG_HYPERVISOR_SUPPORT
    if (current_el != 2) {
        kprintf("CurrentEL is not EL2\n");
        PANIC();
    }
#endif

    /* 内存属性标识 */
    u64 mair;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    //write_sysreg_s(MAIR_EL1_SET, SYS_MAIR_EL2);
    //mair = read_sysreg_s(SYS_MAIR_EL2);
    write_sysreg(MAIR_EL1_SET, MAIR_EL2);
    mair = read_sysreg(MAIR_EL2);
    kprintf("MAIR_EL2 = 0x%lx\n", mair);
#else
    MSR("MAIR_EL1", MAIR_EL1_SET);
    MRS("MAIR_EL1", mair);
    kprintf("MAIR_EL1 = 0x%lx\n", mair);
#endif

    /* 当前Core支持的物理地址范围 */
    u64 mmfr0, pa_range, asid_range;
    MRS("ID_AA64MMFR0_EL1", mmfr0);
    pa_range = bitfield_get(mmfr0, ID_AA64MMFR0_PARANGE_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.PARANGE = 0x%lx\n", pa_range);

    asid_range = bitfield_get(mmfr0, ID_AA64MMFR0_ASID_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.ASIDBITS = 0x%lx\n", asid_range);

    u64 tcr;
#ifdef CONFIG_HYPERVISOR_SUPPORT
    tcr = read_sysreg(TCR_EL2);
    kprintf("TCR_EL2 = 0x%lx\n", tcr);

    write_sysreg((pa_range << TCR_EL2_PS_SHIFT) | \
                            TCR_T0SZ(VA_BITS) | \
                            TCR_TG0_4K | \
                            TCR_SH0_INNER | \
                            TCR_ORGN0_WBWA | \
                            TCR_IRGN0_WBWA, TCR_EL2);

    tcr = read_sysreg(TCR_EL2);
    kprintf("TCR_EL2 = 0x%lx\n", tcr);
#else
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
#endif

    idmap_pt_init();
    boot_identify_mapping();
    boot_kernel_mapping();

    set_ttbr();

    u64 free_pg = idmap_pt_info();
    kprintf("early identify pgtable total pages = %d, free = %d\n", EARLY_PGTABLE_NR_PAGES, free_pg);
}

