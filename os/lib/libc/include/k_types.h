#ifndef __K_TYPES_H
#define __K_TYPES_H

#include <chinos/config.h>

#include <asm/types.h>

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#ifdef CONFIG_ARCH_32
    typedef long long int64_t;
    typedef unsigned long long uint64_t;
    typedef long intptr_t;
    typedef unsigned long uintptr_t;
    typedef unsigned long word_t;
#elif defined(CONFIG_ARCH_64)
    typedef long int64_t;
    typedef unsigned long uint64_t;
    typedef long intptr_t;
    typedef unsigned long uintptr_t;
    typedef unsigned long word_t;
#endif

typedef unsigned long size_t;
typedef long  ssize_t;

typedef int64_t off_t;

typedef int pid_t;

typedef unsigned long asid_t;

#endif
