#ifndef __UAPI_FAULT_H
#define __UAPI_FAULT_H

#include <chinos/config.h>

typedef enum fault_type{
    FAULT_INS = 0,
    FAULT_DATA,
    FAULT_FPU,
} fault_type_t;


#endif
