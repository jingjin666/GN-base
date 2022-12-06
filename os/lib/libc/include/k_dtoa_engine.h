#ifndef __K_DTOA_ENGINE_H
#define __K_DTOA_ENGINE_H

#include <k_types.h>
#include <k_float.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DTOA_MAX_DIG        DBL_DIG

#define DTOA_MINUS          1
#define DTOA_ZERO           2
#define DTOA_INF            4
#define DTOA_NAN            8
#define DTOA_CARRY          16    /* Carry was to master position. */

#if DBL_MAX_10_EXP >= 1 && DBL_MAX_10_EXP < 2
#  define DTOA_SCALE_UP_NUM 1
#endif
#if DBL_MAX_10_EXP >= 2 && DBL_MAX_10_EXP < 4
#  define DTOA_SCALE_UP_NUM 2
#endif
#if DBL_MAX_10_EXP >= 4 && DBL_MAX_10_EXP < 8
#  define DTOA_SCALE_UP_NUM 3
#endif
#if DBL_MAX_10_EXP >= 8 && DBL_MAX_10_EXP < 16
#  define DTOA_SCALE_UP_NUM 4
#endif
#if DBL_MAX_10_EXP >= 16 && DBL_MAX_10_EXP < 32
#  define DTOA_SCALE_UP_NUM 5
#endif
#if DBL_MAX_10_EXP >= 32 && DBL_MAX_10_EXP < 64
#  define DTOA_SCALE_UP_NUM 6
#endif
#if DBL_MAX_10_EXP >= 64 && DBL_MAX_10_EXP < 128
#  define DTOA_SCALE_UP_NUM 7
#endif
#if DBL_MAX_10_EXP >= 128 && DBL_MAX_10_EXP < 256
#  define DTOA_SCALE_UP_NUM 8
#endif
#if DBL_MAX_10_EXP >= 256 && DBL_MAX_10_EXP < 512
#  define DTOA_SCALE_UP_NUM 9
#endif
#if DBL_MAX_10_EXP >= 512 && DBL_MAX_10_EXP < 1024
#  define DTOA_SCALE_UP_NUM 10
#endif
#if DBL_MAX_10_EXP >= 1024 && DBL_MAX_10_EXP < 2048
#  define DTOA_SCALE_UP_NUM 11
#endif
#if DBL_MAX_10_EXP >= 2048 && DBL_MAX_10_EXP < 4096
#  define DTOA_SCALE_UP_NUM 12
#endif
#if DBL_MAX_10_EXP >= 4096 && DBL_MAX_10_EXP < 8192
#  define DTOA_SCALE_UP_NUM 13
#endif
#if DBL_MAX_10_EXP >= 8192 && DBL_MAX_10_EXP < 16384
#  define DTOA_SCALE_UP_NUM 14
#endif
#if DBL_MAX_10_EXP >= 16384 && DBL_MAX_10_EXP < 32768
#  define DTOA_SCALE_UP_NUM 15
#endif
#if DBL_MAX_10_EXP >= 32768 && DBL_MAX_10_EXP < 65536
#  define DTOA_SCALE_UP_NUM 16
#endif
#if DBL_MAX_10_EXP >= 65536 && DBL_MAX_10_EXP < 131072
#  define DTOA_SCALE_UP_NUM 17
#endif
#if DBL_MIN_10_EXP <= -1 && DBL_MIN_10_EXP > -2
#  define DTOA_SCALE_DOWN_NUM 1
#endif
#if DBL_MIN_10_EXP <= -2 && DBL_MIN_10_EXP > -4
#  define DTOA_SCALE_DOWN_NUM 2
#endif
#if DBL_MIN_10_EXP <= -4 && DBL_MIN_10_EXP > -8
#  define DTOA_SCALE_DOWN_NUM 3
#endif
#if DBL_MIN_10_EXP <= -8 && DBL_MIN_10_EXP > -16
#  define DTOA_SCALE_DOWN_NUM 4
#endif
#if DBL_MIN_10_EXP <= -16 && DBL_MIN_10_EXP > -32
#  define DTOA_SCALE_DOWN_NUM 5
#endif
#if DBL_MIN_10_EXP <= -32 && DBL_MIN_10_EXP > -64
#  define DTOA_SCALE_DOWN_NUM 6
#endif
#if DBL_MIN_10_EXP <= -64 && DBL_MIN_10_EXP > -128
#  define DTOA_SCALE_DOWN_NUM 7
#endif
#if DBL_MIN_10_EXP <= -128 && DBL_MIN_10_EXP > -256
#  define DTOA_SCALE_DOWN_NUM 8
#endif
#if DBL_MIN_10_EXP <= -256 && DBL_MIN_10_EXP > -512
#  define DTOA_SCALE_DOWN_NUM 9
#endif
#if DBL_MIN_10_EXP <= -512 && DBL_MIN_10_EXP > -1024
#  define DTOA_SCALE_DOWN_NUM 10
#endif
#if DBL_MIN_10_EXP <= -1024 && DBL_MIN_10_EXP > -2048
#  define DTOA_SCALE_DOWN_NUM 11
#endif
#if DBL_MIN_10_EXP <= -2048 && DBL_MIN_10_EXP > -4096
#  define DTOA_SCALE_DOWN_NUM 12
#endif
#if DBL_MIN_10_EXP <= -4096 && DBL_MIN_10_EXP > -8192
#  define DTOA_SCALE_DOWN_NUM 13
#endif
#if DBL_MIN_10_EXP <= -8192 && DBL_MIN_10_EXP > -16384
#  define DTOA_SCALE_DOWN_NUM 14
#endif
#if DBL_MIN_10_EXP <= -16384 && DBL_MIN_10_EXP > -32768
#  define DTOA_SCALE_DOWN_NUM 15
#endif
#if DBL_MIN_10_EXP <= -32768 && DBL_MIN_10_EXP > -65536
#  define DTOA_SCALE_DOWN_NUM 16
#endif
#if DBL_MIN_10_EXP <= -65536 && DBL_MIN_10_EXP > -131072
#  define DTOA_SCALE_DOWN_NUM 17
#endif

#define DTOA_ROUND_NUM        (DBL_DIG + 1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct dtoa_s
{
  int32_t exp;
  uint8_t flags;
  char digits[DTOA_MAX_DIG + 1];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const double g_dtoa_scale_up[];
extern const double g_dtoa_scale_down[];
extern const double g_dtoa_round[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int __dtoa_engine(double x, struct dtoa_s *dtoa, int max_digits,
                  int max_decimals);

#endif
