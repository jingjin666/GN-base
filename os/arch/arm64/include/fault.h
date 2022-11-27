#ifndef __FAULT_H
#define __FAULT_H

#include <chinos/config.h>

typedef enum fault_type{
    FAULT_INS = 0,
    FAULT_DATA,
} fault_type_t;

void do_page_fault(unsigned long far_el1);

#endif
