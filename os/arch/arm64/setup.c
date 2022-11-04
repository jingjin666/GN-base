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
#include <uapi/util.h>
#include <board.h>

// 存放恒等映射和初始化映射的页表,这一阶段都使用段式映射,页表空间使用较小,这里初始化4M空间来进行存储管理
#define EARLY_PGTABLE_NR_PAGES  1024
#define EARLY_PGTABLE_MEM_SIZE  (PAGE_SIZE * EARLY_PGTABLE_NR_PAGES)

typedef struct mem_region
{
    unsigned long pbase;
    unsigned long vbase;
    unsigned long size;
} mem_region_t;

/* Kernel内存空间映射 */
const struct mem_region kernel_normal_ram[] = {
    {
        .pbase = CONFIG_KERNEL_LOAD_ADDR,
        .vbase = CONFIG_KERNEL_EXEC_ADDR,
        .size = 0,
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
static BOOTDATA struct page_table identifymap_pg_dir;
// 页表空间
static BOOTDATA char early_pgtable_space[EARLY_PGTABLE_MEM_SIZE];
// 页表内存管理的数据结构
static BOOTDATA char early_pgtable_mem[EARLY_PGTABLE_NR_PAGES];

static void BOOTPHYSIC early_pgtable_mem_init(void)
{
    k_memset(early_pgtable_space, 0, sizeof(early_pgtable_space));
    k_memset(early_pgtable_mem, 0, sizeof(early_pgtable_mem));
}

u64 BOOTPHYSIC early_pgtable_alloc(u64 size)
{
    for (int i = 0; i < EARLY_PGTABLE_NR_PAGES; i++) {
        if (early_pgtable_mem[i] == 0) {
            early_pgtable_mem[i] = 1;
            return (u64)&early_pgtable_space[i * PAGE_SIZE];
        }
    }
    return 0;
}

static void BOOTPHYSIC boot_identify_mapping(void)
{
    // 注意此刻内存环境都在物理地址空间下进行
    extern unsigned long boot_physic_start;
    extern unsigned long boot_end;
    u64 vaddr, paddr, end, size, attr;

    vaddr = (u64)&boot_physic_start;
    paddr = (u64)&boot_physic_start;
    end = (u64)&boot_end;
    size = end - paddr;
    attr = pgprot_val(PAGE_KERNEL_EXEC);
    kprintf("%s:%d#pg_map: %p, %p, %p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    pg_map((pgd_t *)&identifymap_pg_dir, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pg_dir);
}

static void BOOTPHYSIC boot_initpgtable_mapping(void)
{
    extern unsigned long kernel_start;
    extern unsigned long kernel_end;
    u64 vaddr, paddr, size, attr;

    // 映射内核普通内存
    vaddr = kernel_normal_ram[0].vbase;
    paddr = kernel_normal_ram[0].pbase;
    size = (u64)&kernel_end - (u64)&kernel_start;
    attr = pgprot_val(PAGE_KERNEL_EXEC);
    kprintf("%s:%d#pg_map: %p, %p, %p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    pg_map((pgd_t *)&identifymap_pg_dir, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pg_dir);

    // 映射内核设备内存
    vaddr = kernel_dev_ram[0].vbase;
    paddr = kernel_dev_ram[0].pbase;
    size = kernel_dev_ram[0].size;
    attr = PROT_SECT_DEVICE_nGnRE;
    kprintf("%s:%d#pg_map: %p, %p, %p, %p\n", __FUNCTION__, __LINE__, vaddr, paddr, size, attr);
    pg_map((pgd_t *)&identifymap_pg_dir, vaddr, paddr, size, attr, early_pgtable_alloc);

    //dump_pgtable_verbose((pgd_t *)&identifymap_pg_dir);

    /* TTBR0 */
    u64 ttbr0;
    MSR("TTBR0_EL1", (u64)&identifymap_pg_dir);
    MRS("TTBR0_EL1", ttbr0);
    kprintf("TTBR0_EL1 = 0x%lx\n", ttbr0);

    /* TTBR1 */
    u64 ttbr1;
    MSR("TTBR1_EL1", (u64)&identifymap_pg_dir);
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
    u64 mmfr0, pa_range;
    MRS("ID_AA64MMFR0_EL1", mmfr0);
    pa_range = bitfield_get(mmfr0, ID_AA64MMFR0_PARANGE_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.PARANGE = 0x%lx\n", pa_range);

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

    early_pgtable_mem_init();
    boot_identify_mapping();
    boot_initpgtable_mapping();
}

