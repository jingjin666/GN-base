#include <k_stdio.h>
#include <uapi/util.h>
#include <instructionset.h>

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

    // 重定位中断向量表
    relocate_vector();
}
