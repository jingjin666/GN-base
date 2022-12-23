#ifndef __ASM_FAULT_H
#define __ASM_FAULT_H

#ifndef __ASSEMBLY__

typedef enum fault_type{
    FAULT_INS = 0,
    FAULT_DATA,
} fault_type_t;

void do_page_fault(unsigned long far_el1);

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_FAULT_H */
