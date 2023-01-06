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

/* Kernel内存空间映射 */
struct mem_region kernel_normal_ram[] = {
    {
        .pbase = RAM_PBASE,
        .vbase = RAM_VBASE,
        .size = RAM_SIZE,
    },
    {.size = 0}, // end of regions
};

/* Kernel设备空间映射*/
struct mem_region kernel_dev_ram[] = {
    // MMIO
    {
        .pbase = MMIO_PBASE,
        .vbase = MMIO_VBASE,
        .size =  MMIO_SIZE
    },
    {.size = 0}, // end of regions
};

