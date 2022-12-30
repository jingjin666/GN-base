#include <chinos/config.h>

#include <k_stdio.h>
#include <uapi/util.h>
#include <instructionset.h>
#include <sysreg.h>
#ifdef CONFIG_HYPERVISOR_SUPPORT
#include <vcpu.h>

/* Trap WFI/WFE/SMC and override CPSR.AIF */
#define HCR_COMMON ( HCR_TWI | HCR_TWE | HCR_VM | HCR_RW | HCR_AMO | HCR_IMO | HCR_FMO )

/* Allow native tasks to run at EL0, but restrict access */
#define HCR_NATIVE ( HCR_COMMON | HCR_TGE | HCR_TVM | HCR_TTLB | HCR_DC \
                   | HCR_TAC | HCR_SWIO |  HCR_TSC )
#define HCR_VCPU   ( HCR_COMMON | HCR_TSC)

#endif

#include "cpu.h"

extern void vector(void);
static void relocate_vector(void)
{
    unsigned long current_el;
    unsigned long vbase = (unsigned long)vector;

    MRS("CurrentEL", current_el);
    current_el = bitfield_get(current_el, 2, 2);
    switch(current_el)
    {
        case 0:
            break;
        case 1:
            MSR("VBAR_EL1", vbase);
            MRS("VBAR_EL1", vbase);
            kprintf("VBAR_EL1 = %p\n", vbase);
            break;
        case 2:
            MSR("VBAR_EL2", vbase);
            MRS("VBAR_EL2", vbase);
            kprintf("VBAR_EL2 = %p\n", vbase);
            break;
        case 3:
            MSR("VBAR_EL3", vbase);
            MRS("VBAR_EL3", vbase);
            kprintf("VBAR_EL3 = %p\n", vbase);
            break;
        default:
            break;
    }
}

void cpu_init(void)
{
    unsigned long spsel;
    MRS("SPSel", spsel);
    kprintf("SPSel = %p\n", spsel);

#ifdef CONFIG_HYPERVISOR_SUPPORT
    /* 当前Core支持的物理地址范围 */
    u64 mmfr0, pa_range;
    MRS("ID_AA64MMFR0_EL1", mmfr0);
    pa_range = bitfield_get(mmfr0, ID_AA64MMFR0_PARANGE_SHIFT, 4);
    kprintf("ID_AA64MMFR0_EL1.PARANGE = 0x%lx\n", pa_range);

    u64 vtcr;
    vtcr = read_sysreg(VTCR_EL2);
    kprintf("VTCR_EL2 = 0x%lx\n", vtcr);
    
    vtcr =  (pa_range << VTCR_EL2_PS_SHIFT);    // 40-bit PA size
    vtcr |= VTCR_EL2_T0SZ(VA_BITS);             // 39-bit input IPA
    vtcr |= (1 << VTCR_EL2_SL0_SHIFT);          // 4KiB, start at level 1
    vtcr |= VTCR_EL2_TG0_4K;                    // 4KiB page size
    vtcr |= VTCR_EL2_SH0_INNER;                 // inner shareable
    vtcr |= VTCR_EL2_ORGN0_WBWA;                // outer write-back, read/write allocate
    vtcr |= VTCR_EL2_IRGN0_WBWA;                // inner write-back, read/write allocate
    write_sysreg(vtcr, VTCR_EL2);

    vtcr = read_sysreg(VTCR_EL2);
    kprintf("VTCR_EL2 = 0x%lx\n", vtcr);

    /* 配置HCR */
    u64 hcr_el2;
    hcr_el2 = read_sysreg(HCR_EL2);
    kprintf("HCR_EL2 = %p\n", hcr_el2);

    write_sysreg(HCR_NATIVE, HCR_EL2);

    hcr_el2 = read_sysreg(HCR_EL2);
    kprintf("HCR_EL2 = %p\n", hcr_el2);
#endif

    // 重定位中断向量表
    relocate_vector();
}
