#ifndef __ASM_REGISTERSET_H
#define __ASM_REGISTERSET_H

#include <chinos/config.h>

/* CurrentEL register */
#define PEXPL1                  (1 << 2)
#define PEXPL2                  (1 << 3)

/* PSTATE register */
#define PMODE_FIRQ              (1 << 6)
#define PMODE_IRQ               (1 << 7)
#define PMODE_SERROR            (1 << 8)
#define PMODE_DEBUG             (1 << 9)
#define PMODE_EL0t              0
#define PMODE_EL1t              4
#define PMODE_EL1h              5
#define PMODE_EL2h              9

/* DAIF register */
#define DAIF_FIRQ               (1 << 6)
#define DAIF_IRQ                (1 << 7)
#define DAIF_SERROR             (1 << 8)
#define DAIF_DEBUG              (1 << 9)
#define DAIFSET_MASK            0xf

#define PSTATE_USER         (PMODE_FIRQ | PMODE_EL0t | PMODE_SERROR)
#ifdef CONFIG_HYPERVISOR_SUPPORT
#define PSTATE_VCPU         (PMODE_FIRQ | PMODE_EL1h | PMODE_SERROR)
#define PSTATE_KERNEL       (PMODE_FIRQ | PMODE_EL2h | PMODE_SERROR)
#else
#define PSTATE_KERNEL       (PMODE_FIRQ | PMODE_EL1h | PMODE_SERROR)
#endif

/* CPACR_EL1 register */
#define CPACR_EL1_FPEN          20     // FP regiters access

/* Offsets within the user context, these need to match the order in context_t below */
#define CTX_SIZE                (34*8)
#define CTX_OFFS_LR             (30*8)
#define CTX_OFFS_PC             (31*8)
#define CTX_OFFS_SP             (32*8)
#define CTX_OFFS_PSTATE         (33*8)

#endif /* __ASM_REGISTERSET_H */
