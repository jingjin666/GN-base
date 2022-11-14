#include "k_dtoa_engine.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INFINITY    (1.0/0.0)
#define NAN         (0.0/0.0)
#define HUGE_VAL    INFINITY

#define INFINITY_F  (1.0F/0.0F)
#define NAN_F       (0.0F/0.0F)

#define isnan(x)    ((x) != (x))
#define isinf(x)    (((x) == INFINITY) || ((x) == -INFINITY))
#define isfinite(x) (!(isinf(x) || isnan(x)))

#define isinf_f(x)  (((x) == INFINITY_F) || ((x) == -INFINITY_F))


/* A bit of CPP trickery -- construct the floating-point value 10 ** DBL_DIG
 * by pasting the value of DBL_DIG onto '1e' to
 */

#define PASTE(a)      1e##a
#define SUBSTITUTE(a) PASTE(a)
#define MIN_MANT      (SUBSTITUTE(DBL_DIG))
#define MAX_MANT      (10.0 * MIN_MANT)
#define MIN_MANT_INT  ((uint64_t)MIN_MANT)
#define MIN_MANT_EXP  DBL_DIG

#define MAX(a, b)     ((a) > (b) ? (a) : (b))
#define MIN(a, b)     ((a) < (b) ? (a) : (b))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int __dtoa_engine(double x, struct dtoa_s *dtoa, int max_digits,
                  int max_decimals)
{
  int32_t exp = 0;
  uint8_t flags = 0;
  int i;

  if (__builtin_signbit(x))
    {
      flags |= DTOA_MINUS;
      x = -x;
    }

  if (x == 0)
    {
      flags |= DTOA_ZERO;
      for (i = 0; i < max_digits; i++)
        dtoa->digits[i] = '0';
    }
  else if (isnan(x))
    {
      flags |= DTOA_NAN;
    }
  else if (isinf(x))
    {
      flags |= DTOA_INF;
    }
  else
    {
      double y;

      exp = MIN_MANT_EXP;

      /* Bring x within range MIN_MANT <= x < MAX_MANT while computing
       * exponent value
       */

      if (x < MIN_MANT)
        {
          for (i = DTOA_SCALE_UP_NUM - 1; i >= 0; i--)
            {
              y = x * g_dtoa_scale_up[i];
              if (y < MAX_MANT)
                {
                  x = y;
                  exp -= (1 << i);
                }
            }
        }
      else
        {
          for (i = DTOA_SCALE_DOWN_NUM - 1; i >= 0; i--)
            {
              y = x * g_dtoa_scale_down[i];
              if (y >= MIN_MANT)
                {
                  x = y;
                  exp += (1 << i);
                }
            }
        }

      /* If limiting decimals, then limit the max digits to no more than the
       * number of digits left of the decimal plus the number of digits right
       * of the decimal
       */

      if (max_decimals != 0)
        {
          max_digits = MIN(max_digits, max_decimals + MAX(exp + 1, 1));
        }

      /* Round nearest by adding 1/2 of the last digit before converting to
       * int. Check for overflow and adjust mantissa and exponent values
       */

      x = x + g_dtoa_round[max_digits];

      if (x >= MAX_MANT)
        {
          x /= 10.0;
          exp++;
        }

      /* Now convert mantissa to decimal. */

      uint64_t mant = (uint64_t) x;
      uint64_t decimal = MIN_MANT_INT;

      /* Compute digits */

      for (i = 0; i < max_digits; i++)
        {
          dtoa->digits[i] = mant / decimal + '0';
          mant %= decimal;
          decimal /= 10;
        }
    }

  dtoa->digits[max_digits] = '\0';
  dtoa->flags = flags;
  dtoa->exp = exp;
  return max_digits;
}
