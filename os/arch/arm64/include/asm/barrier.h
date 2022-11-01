#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H

#ifndef __ASSEMBLY__

#define isb()   asm volatile("isb sy" : : : "memory")
#define dsb()   asm volatile("dsb sy" : : : "memory")
#define dmb()   asm volatile("dmb sy" : : : "memory")

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_BARRIER_H */
