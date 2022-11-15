#ifndef __ARMV8A_CACHE_H
#define __ARMV8A_CACHE_H

#include <chinos/config.h>

#include <barrier.h>

static inline long clzl(unsigned long x)
{
#ifdef CONFIG_CLZ_NO_BUILTIN
#if CONFIG_WORD_SIZE == 32
    return __clzsi2(x);
#else
    return __clzdi2(x);
#endif
#else
    return __builtin_clzl(x);
#endif
}

static inline void cleanByVA(unsigned long vaddr, unsigned long paddr)
{
    asm volatile("dc cvac, %0" : : "r"(vaddr));
    dmb();
}

static inline void cleanByVA_PoU(unsigned long vaddr, unsigned long paddr)
{
    asm volatile("dc cvau, %0" : : "r"(vaddr));
    dmb();
}

static inline void invalidateByVA(unsigned long vaddr, unsigned long paddr)
{
    asm volatile("dc ivac, %0" : : "r"(vaddr));
    dmb();
}

static inline void invalidateByVA_I(unsigned long vaddr, unsigned long paddr)
{
    asm volatile("ic ivau, %0" : : "r"(vaddr));
    dsb();
    isb();
}

static inline void invalidate_I_PoU(void)
{
#if 0//CONFIG_MAX_CPU_NUM > 1
    asm volatile("ic ialluis");
#else
    asm volatile("ic iallu");
#endif
    isb();
}

static inline void cleanInvalByVA(unsigned long vaddr, unsigned long paddr)
{
    asm volatile("dc civac, %0" : : "r"(vaddr));
    dsb();
}

static inline void branchFlush(unsigned long vaddr, unsigned long paddr)
{

}

static inline void cleanByWSL(unsigned long wsl)
{
    asm volatile("dc csw, %0" : : "r"(wsl));
}

static inline void cleanInvalidateByWSL(unsigned long wsl)
{
    asm volatile("dc cisw, %0" : : "r"(wsl));
}

static inline unsigned long readCLID(void)
{
    unsigned long CLID;
    MRS("clidr_el1", CLID);
    return CLID;
}

#endif /* __ARMV8A_CACHE_H */
