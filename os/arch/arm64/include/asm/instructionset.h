#ifndef __ASM_INSTRUCTIONSET_H
#define __ASM_INSTRUCTIONSET_H

#include <k_types.h>

#ifndef __ASSEMBLY__

#define sev()       asm volatile("sev" : : : "memory")
#define wfe()       asm volatile("wfe" : : : "memory")
#define wfi()       asm volatile("wfi" : : : "memory")

#define MRS(reg, v) asm volatile("mrs %x0," reg : "=r"(v))
#define MSR(reg, v)                                \
    do {                                           \
        word_t _v = v;                             \
        asm volatile("msr " reg ",%x0" :: "r" (_v));\
    }while(0)

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_INSTRUCTIONSET_H */
