#ifndef __K_STDDEF_H
#define __K_STDDEF_H

#include <chinos/config.h>

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif

typedef long unsigned int size_t;
typedef long int ssize_t;

enum
{
    ERROR = -1,
    OK = 0,
};

#endif