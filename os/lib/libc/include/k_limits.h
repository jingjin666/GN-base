#ifndef __K_LIMITS_H
#define __K_LIMITS_H

#include <chinos/config.h>

#define SCHAR_MIN  (-SCHAR_MAX - 1)
#define SCHAR_MAX   127
#define UCHAR_MAX   255

#define CHAR_MIN    0
#define CHAR_MAX    UCHAR_MAX

#define SHRT_MIN    (-SHRT_MAX - 1)
#define SHRT_MAX    32767
#define USHRT_MAX   65535U

#define INT_MIN     (-INT_MAX - 1)
#define INT_MAX     2147483647
#define UINT_MAX    4294967295U

#ifdef CONFIG_ARCH_32

#define LONG_MIN    (-LONG_MAX - 1)
#define LONG_MAX    2147483647L
#define ULONG_MAX   4294967295UL

#define LLONG_MIN   (-LLONG_MAX - 1)
#define LLONG_MAX   9223372036854775807LL
#define ULLONG_MAX  18446744073709551615ULL

#define WORD_BITS   (32)
#define WORD_SHIFT  (5)

#else // CONFIG_ARCH_64

#define LONG_MIN    (-LONG_MAX - 1)
#define LONG_MAX    9223372036854775807LL
#define ULONG_MAX   18446744073709551615ULL

#define LLONG_MIN   (-LLONG_MAX - 1)
#define LLONG_MAX   9223372036854775807LL
#define ULLONG_MAX  18446744073709551615ULL

#define WORD_BITS   (64)
#define WORD_SHIFT  (6)

#endif // CONFIG_ARCH_32


#endif
