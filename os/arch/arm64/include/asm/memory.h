#ifndef __ASM_MEMORY_H
#define __ASM_MEMORY_H

#ifndef __ASSEMBLY__
#include <chinos/config.h>
#include <sysreg.h>

#define VA_BITS			(CONFIG_ARM64_VA_BITS)

/*
 * Memory types available.
 */
#define MT_DEVICE_nGnRnE	0
#define MT_DEVICE_nGnRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		    4
#define MT_NORMAL_WT		5

#define MAIR_EL1_SET \
    (MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRnE, MT_DEVICE_nGnRnE) | \
    MAIR_ATTRIDX(MAIR_ATTR_DEVICE_nGnRE, MT_DEVICE_nGnRE) | \
    MAIR_ATTRIDX(MAIR_ATTR_DEVICE_GRE, MT_DEVICE_GRE) | \
    MAIR_ATTRIDX(MAIR_ATTR_NORMAL_NC, MT_NORMAL_NC) | \
    MAIR_ATTRIDX(MAIR_ATTR_NORMAL, MT_NORMAL) | \
    MAIR_ATTRIDX(MAIR_ATTR_NORMAL_WT, MT_NORMAL_WT))
#endif
#endif /* __ASM_MEMORY_H */
