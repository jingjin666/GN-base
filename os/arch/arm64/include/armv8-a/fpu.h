#ifndef __ARMV8A_FPU_H
#define __ARMV8A_FPU_H

#include <chinos/config.h>

#include <k_stdint.h>
#include <const.h>
#include <instructionset.h>
#include <registerset.h>
#include <uapi/util.h>

#ifdef CONFIG_ARCH_FPU

/* Trap any FPU related instructions to EL2 */
static inline void enable_trapfpu(void)
{
    word_t cptr;
    MRS("cptr_el2", cptr);
    cptr |= (bit(10) | bit(31));
    MSR("cptr_el2", cptr);
}

/* Disable trapping FPU instructions to EL2 */
static inline void disable_trapfpu(void)
{
    word_t cptr;
    MRS("cptr_el2", cptr);
    cptr &= ~(bit(10) | bit(31));
    MSR("cptr_el2", cptr);
}

/* Enable FPU access in EL0 and EL1 */
static inline void enable_fpu_EL01(void)
{
    word_t cpacr;
    MRS("cpacr_el1", cpacr);
    cpacr |= (3 << CPACR_EL1_FPEN);
    MSR("cpacr_el1", cpacr);
}

/* Disable FPU access in EL0 */
static inline void disable_fpu_EL0(void)
{
    word_t cpacr;
    MRS("cpacr_el1", cpacr);
    cpacr &= ~(3 << CPACR_EL1_FPEN);
    cpacr |= (1 << CPACR_EL1_FPEN);
    MSR("cpacr_el1", cpacr);
}

#endif /* CONFIG_ARCH_FPU */

#endif /* __ARMV8A_FPU_H */
