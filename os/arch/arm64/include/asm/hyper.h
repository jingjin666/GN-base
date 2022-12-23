#ifndef __ASM_HYPER_H
#define __ASM_HYPER_H

#ifndef __ASSEMBLY__

#include <chinos/config.h>

#include <vcpu.h>
#include <sysreg.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT

static inline u8 has_vhe(void)
{
    u64 hcr_el2;
    hcr_el2 = read_sysreg(HCR_EL2);
    return hcr_el2 & HCR_E2H;
}

#endif

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_HYPER_H */
