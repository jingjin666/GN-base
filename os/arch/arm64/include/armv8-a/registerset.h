#ifndef __ARMV8A_REGISTERSET_H
#define __ARMV8A_REGISTERSET_H

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

/* ESR register */
#define ESR_EC_SHIFT            26
#define ESR_EC_LEL_DABT         0x24    // Data abort from a lower EL
#define ESR_EC_CEL_DABT         0x25    // Data abort from the current EL
#define ESR_EC_LEL_IABT         0x20    // Instruction abort from a lower EL
#define ESR_EC_CEL_IABT         0x21    // Instruction abort from the current EL
#define ESR_EC_LEL_SVC64        0x15    // SVC from a lower EL in AArch64 state
#define ESR_EC_LEL_HVC64        0x16    // HVC from EL1 in AArch64 state
#define ESR_EL1_EC_ENFP         0x7     // Access to Advanced SIMD or floating-point registers

#define PSTATE_USER         (PMODE_FIRQ | PMODE_EL0t | PMODE_SERROR)
#define PSTATE_KERNEL       (PMODE_FIRQ | PMODE_EL1h | PMODE_SERROR)

/* ID_AA64PFR0_EL1 register */
#define ID_AA64PFR0_EL1_FP      16     // HWCap for Floating Point
#define ID_AA64PFR0_EL1_ASIMD   20     // HWCap for Advanced SIMD

/* CPACR_EL1 register */
#define CPACR_EL1_FPEN          20     // FP regiters access

/* Offsets within the user context, these need to match the order in context_t below */
#define CTX_SIZE                (35*8)
#define CTX_OFFS_LR             (30*8)
#define CTX_OFFS_PC             (31*8)
#define CTX_OFFS_SP             (32*8)
#define CTX_OFFS_PSTATE         (33*8)
#define CTX_OFFS_TPIDRURO       (34*8)


#endif /* __ARMV8A_REGISTERSET_H */
