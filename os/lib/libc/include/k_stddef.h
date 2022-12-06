#ifndef __K_STDDEF_H
#define __K_STDDEF_H

#include <chinos/config.h>

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif

#include <k_types.h>

#endif
