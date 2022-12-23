#ifndef __ASM_TLB_H
#define __ASM_TLB_H

#include <chinos/config.h>

#include <k_assert.h>
#include <const.h>
#include <barrier.h>

static inline void flush_TLB_EL2(void)
{
    asm volatile("tlbi alle2");
}

static inline void flush_TLB_EL1(void)
{
    asm volatile("tlbi alle1");
}

static inline void flush_TLB(void)
{
    dsb();
    asm volatile("tlbi vmalle1");
    dsb();
    isb();
}

static inline void flush_TLB_ASID(unsigned long asid)
{
    assert(asid < BIT(16));

    dsb();
    asm volatile("tlbi aside1, %0" : : "r"(asid << 48));
    dsb();
    isb();
}

static inline void flush_TLB_VAASID(unsigned long mva_plus_asid)
{
    dsb();
    asm volatile("tlbi vae1, %0" : : "r"(mva_plus_asid));
    dsb();
    isb();
}

/* Flush all stage 1 and stage 2 translations used at
 * EL1 with the current VMID which is specified by vttbr_el2 */
static inline void flush_TLB_VMALLS12E1(void)
{
    asm volatile("tlbi vmalls12e1");
    dsb();
    isb();
}

/* Flush IPA with the current VMID */
static inline void flush_TLB_IPA(unsigned long ipa)
{
    asm volatile("tlbi ipas2e1, %0" :: "r"(ipa));
    dsb();
    asm volatile("tlbi vmalle1");
    dsb();
    isb();
}

void lockTLBEntry(unsigned long vaddr);

#endif /* __ASM_TLB_H */
