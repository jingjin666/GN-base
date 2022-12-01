#ifndef __K_STDINT_H
#define __K_STDINT_H

#include <chinos/config.h>

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#ifdef CONFIG_ARCH_32
    typedef long long int64_t;
    typedef unsigned long long uint64_t;
    typedef int intptr_t;
    typedef unsigned int uintptr_t;
    typedef unsigned long word_t;
#elif defined(CONFIG_ARCH_64)
    typedef long int64_t;
    typedef unsigned long uint64_t;
    typedef long intptr_t;
    typedef unsigned long uintptr_t;
    typedef unsigned long word_t;
#endif

typedef int64_t off_t;

typedef int16_t pid_t;
typedef int16_t tid_t;

typedef uint64_t asid_t;

#endif
