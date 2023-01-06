#include <chinos/config.h>

#include <kernel.h>
#include <k_stdarg.h>
#include <k_stdint.h>
#include <k_stddef.h>
#include <k_limits.h>
#include <k_float.h>

struct va_format
{
    const char *fmt;
    va_list *va;
};

#define getreg64(a)           (*(volatile uint64_t *)(a))
#define putreg64(v,a)         (*(volatile uint64_t *)(a) = (v))

static BOOTDATA uint64_t uart_base;

static BOOTPHYSIC void early_putchar(char c)
{
    if (c == '\n') {
        early_putchar('\r');
    }

    while ((getreg64(uart_base + 0x018) & (1 << 5)) != 0);
    putreg64(c, uart_base + 0x000);
}

static BOOTPHYSIC size_t early_strnlen(const char *s, size_t maxlen)
{
    const char *sc;
    for (sc = s; maxlen != 0 && *sc != '\0'; maxlen--, ++sc);
    return sc - s;
}

static BOOTPHYSIC float fabsf(float x)
{
    return ((x < 0) ? -x : x);
}

/* Order is relevant here and matches order in format string */
#define FL_ZFILL           0x0001
#define FL_PLUS            0x0002
#define FL_SPACE           0x0004
#define FL_LPAD            0x0008
#define FL_ALT             0x0010

#define FL_ARGNUMBER       0x0020
#define FL_ASTERISK        0x0040

#define FL_WIDTH           0x0080
#define FL_PREC            0x0100

#define FL_LONG            0x0200
#define FL_SHORT           0x0400
#define FL_REPD_TYPE       0x0800

#define FL_NEGATIVE        0x1000

/* The next 2 groups are Exclusive Or */
#define FL_ALTUPP          0x2000
#define FL_ALTHEX          0x4000

#define FL_FLTUPP          0x2000
#define FL_FLTEXP          0x4000
#define FL_FLTFIX          0x8000

#define TYPE_INT           1
#define TYPE_LONG          2
#define TYPE_LONG_LONG     3
#define TYPE_DOUBLE        4
#define TYPE_CHAR_POINTER  5

#define fmt_char(fmt)   (*(fmt)++)
#define fmt_ungetc(fmt)   ((fmt)--)

static BOOTDATA char g_nullstring[] = "(null)";

/* '__ftoa_engine' return next flags (in buf[0]):	*/
#define	FTOA_MINUS	1
#define	FTOA_ZERO	2
#define	FTOA_INF	4
#define	FTOA_NAN	8
#define	FTOA_CARRY	16	/* Carry was to master position.	*/

/* Next flags are to use with `base'. Unused fields are reserved.	*/
#define XTOA_PREFIX	0x0100	/* put prefix for octal or hex	*/
#define XTOA_UPPER	0x0200	/* use upper case letters	*/

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

static BOOTDATA double g_dtoa_scale_up[] =
{
#if DBL_MAX_10_EXP >= 1
  1e1,
#endif
#if DBL_MAX_10_EXP >= 2
  1e2,
#endif
#if DBL_MAX_10_EXP >= 4
  1e4,
#endif
#if DBL_MAX_10_EXP >= 8
  1e8,
#endif
#if DBL_MAX_10_EXP >= 16
  1e16,
#endif
#if DBL_MAX_10_EXP >= 32
  1e32,
#endif
#if DBL_MAX_10_EXP >= 64
  1e64,
#endif
#if DBL_MAX_10_EXP >= 128
  1e128,
#endif
#if DBL_MAX_10_EXP >= 256
  1e256,
#endif
#if DBL_MAX_10_EXP >= 512
  1e512,
#endif
#if DBL_MAX_10_EXP >= 1024
  1e1024,
#endif
#if DBL_MAX_10_EXP >= 2048
  1e2048,
#endif
#if DBL_MAX_10_EXP >= 4096
  1e4096,
#endif
#if DBL_MAX_10_EXP >= 8192
  1e8192,
#endif
#if DBL_MAX_10_EXP >= 16384
  1e16384,
#endif
#if DBL_MAX_10_EXP >= 32768
  1e32768,
#endif
#if DBL_MAX_10_EXP >= 65536
  1e65536,
#endif
};

static BOOTDATA double g_dtoa_scale_down[] =
{
#if DBL_MIN_10_EXP <= -1
  1e-1,
#endif
#if DBL_MIN_10_EXP <= -2
  1e-2,
#endif
#if DBL_MIN_10_EXP <= -4
  1e-4,
#endif
#if DBL_MIN_10_EXP <= -8
  1e-8,
#endif
#if DBL_MIN_10_EXP <= -16
  1e-16,
#endif
#if DBL_MIN_10_EXP <= -32
  1e-32,
#endif
#if DBL_MIN_10_EXP <= -64
  1e-64,
#endif
#if DBL_MIN_10_EXP <= -128
  1e-128,
#endif
#if DBL_MIN_10_EXP <= -256
  1e-256,
#endif
#if DBL_MIN_10_EXP <= -512
  1e-512,
#endif
#if DBL_MIN_10_EXP <= -1024
  1e-1024,
#endif
#if DBL_MIN_10_EXP <= -2048
  1e-2048,
#endif
#if DBL_MIN_10_EXP <= -4096
  1e-4096,
#endif
#if DBL_MIN_10_EXP <= -8192
  1e-8192,
#endif
#if DBL_MIN_10_EXP <= -16384
  1e-16384,
#endif
#if DBL_MIN_10_EXP <= -32768
  1e-32768,
#endif
#if DBL_MIN_10_EXP <= -65536
  1e-65536,
#endif
};

static BOOTDATA double g_dtoa_round[] =
{
#if DBL_DIG >= 30
  5e30,
#endif
#if DBL_DIG >= 29
  5e29,
#endif
#if DBL_DIG >= 28
  5e28,
#endif
#if DBL_DIG >= 27
  5e27,
#endif
#if DBL_DIG >= 26
  5e26,
#endif
#if DBL_DIG >= 25
  5e25,
#endif
#if DBL_DIG >= 24
  5e24,
#endif
#if DBL_DIG >= 23
  5e23,
#endif
#if DBL_DIG >= 22
  5e22,
#endif
#if DBL_DIG >= 21
  5e21,
#endif
#if DBL_DIG >= 20
  5e20,
#endif
#if DBL_DIG >= 19
  5e19,
#endif
#if DBL_DIG >= 18
  5e18,
#endif
#if DBL_DIG >= 17
  5e17,
#endif
#if DBL_DIG >= 16
  5e16,
#endif
#if DBL_DIG >= 15
  5e15,
#endif
#if DBL_DIG >= 14
  5e14,
#endif
#if DBL_DIG >= 13
  5e13,
#endif
#if DBL_DIG >= 12
  5e12,
#endif
#if DBL_DIG >= 11
  5e11,
#endif
#if DBL_DIG >= 10
  5e10,
#endif
#if DBL_DIG >= 9
  5e9,
#endif
#if DBL_DIG >= 8
  5e8,
#endif
#if DBL_DIG >= 7
  5e7,
#endif
#if DBL_DIG >= 6
  5e6,
#endif
#if DBL_DIG >= 5
  5e5,
#endif
#if DBL_DIG >= 4
  5e4,
#endif
#if DBL_DIG >= 3
  5e3,
#endif
#if DBL_DIG >= 2
  5e2,
#endif
#if DBL_DIG >= 1
  5e1,
#endif
#if DBL_DIG >= 0
  5e0,
#endif
};

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

static BOOTPHYSIC int early__dtoa_engine(double x, struct dtoa_s *dtoa, int max_digits,
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

#ifdef CONFIG_LIBC_LONG_LONG
static BOOTPHYSIC char *early__ultoa_invert(unsigned long long val, char *str, int base)
#else
static BOOTPHYSIC char *early__ultoa_invert(unsigned long val, char *str, int base)
#endif
{
  int upper = 0;

  if (base & XTOA_UPPER)
    {
      upper = 1;
      base &= ~XTOA_UPPER;
    }

  do
    {
      int v;

      v   = val % base;
      val = val / base;

      if (v <= 9)
        {
          v += '0';
        }
      else
        {
          if (upper)
            v += 'A' - 10;
          else
            v += 'a' - 10;
        }

      *str++ = v;
    }
  while (val);

  return str;
}

static BOOTPHYSIC char * early_ulltoa_invert (uint64_t val, char *s, uint8_t base)
{
	if (base == 8) {
		do {
			*s = '0' + (val & 0x7);
			val >>= 3;
		} while(val);
		return s;
	}

	if (base == 16) {
		do {
			uint8_t digit = '0' + (val & 0xf);
#if XTOA_UPPER == 0
			if (digit > '0' + 9)
				digit += ('a' - '0' - 10);
#else
			if (digit > '0' + 9)
				digit += ('A' - '0' - 10);
#endif
			*s++ = digit;
			val >>= 4;
		} while(val);
		return s;
	}

	// Every base which in not hex and not oct is considered decimal.

	// 64 bits is not actually enough, we need 65, but it should
	// be good enough for the log dumping we're using this for
	uint64_t xval = val;
	do {
		uint8_t saved = xval;
		xval &= ~1;
		xval += 2;
		xval += xval >> 1;		// *1.5
		xval += xval >> 4;		// *1.0625
		xval += xval >> 8;		// *1.00390625
		xval += xval >> 16;		// *1.000015259
		xval += xval >> 32;		// it all amounts to *1.6
		xval >>= 4;				// /16 ... so *1.6/16 is /10, fraction truncated.
		*s++ = '0' + saved - 10 * (uint8_t)xval;
	} while (xval);
	return s;
}

static BOOTPHYSIC int early_vprintf(const char *fmt, va_list ap)
{
  unsigned char c; /* Holds a char from the format string */
  uint16_t flags;
  int width;
  int prec;
  union
  {
#if defined (CONFIG_LIBC_LONG_LONG) || (ULONG_MAX > 4294967295UL)
    unsigned char __buf[22]; /* Size for -1 in octal, without '\0' */
#else
    unsigned char __buf[11]; /* Size for -1 in octal, without '\0' */
#endif
#ifdef CONFIG_LIBC_FLOATINGPOINT
    struct dtoa_s __dtoa;
#endif
  } u;

#define buf     (u.__buf)
#define _dtoa   (u.__dtoa)

  const char *pnt;
  size_t size;
  unsigned char len;
  int total_len = 0;

#ifdef CONFIG_LIBC_NUMBERED_ARGS

  int argnumber;

#endif

  for (; ; )
    {
      for (; ; )
        {
          c = fmt_char(fmt);
          if (c == '\0')
            {
              goto ret;
            }

          if (c == '%')
            {
              c = fmt_char(fmt);
              if (c != '%')
                {
                  break;
                }
            }

#ifdef CONFIG_LIBC_NUMBERED_ARGS
          if (stream != NULL)
            {
              early_putchar(c);
            }
#else
          early_putchar(c);
#endif
        }

      flags = 0;
      width = 0;
      prec  = 0;

      do
        {
          if (flags < FL_ASTERISK)
            {
              switch (c)
                {
                case '0':
                  flags |= FL_ZFILL;
                  continue;

                case '+':
                  flags |= FL_PLUS;

                  /* FALLTHROUGH */

                case ' ':
                  flags |= FL_SPACE;
                  continue;

                case '-':
                  flags |= FL_LPAD;
                  continue;

                case '#':
                  flags |= FL_ALT;
                  continue;
                }
            }

          if (flags < FL_LONG)
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if (c == '$')
                {
                  if ((flags & FL_ARGNUMBER) == 0)
                    {
                      /* No other flag except FL_WIDTH or FL_ZFILL (leading
                       * zeros) and argument number must be at least 1
                       */

                      if ((flags & ~(FL_WIDTH | FL_ZFILL)) != 0 ||
                          width == 0)
                        {
                          goto ret;
                        }

                      /* It had been the argument number. */

                      argnumber = width;
                      width     = 0;
                      flags     = FL_ARGNUMBER;
                    }
                  else if ((flags & FL_ASTERISK) != 0)
                    {
                      int index;

                      flags    &= ~FL_ASTERISK;

                      if ((flags & FL_PREC) == 0)
                        {
                          index = width;
                        }
                      else
                        {
                          index = prec;
                        }

                      if (index > 0 && index <= numargs)
                        {
                          if (stream == NULL)
                            {
                              arglist[index - 1].type = TYPE_INT;
                              if (index > total_len)
                                {
                                  total_len = index;
                                }
                            }
                          else
                            {
                              if ((flags & FL_PREC) == 0)
                                {
                                  width = (int)arglist[index - 1].value.u;
                                }
                              else
                                {
                                  prec = (int)arglist[index - 1].value.u;
                                }
                            }
                        }
                      else
                        {
                          goto ret;
                        }
                    }
                  else
                    {
                      goto ret;
                    }

                  continue;
                }
#endif

              if (c >= '0' && c <= '9')
                {
                  c -= '0';
                  if ((flags & FL_PREC) != 0)
                    {
                      prec = 10 * prec + c;
                      continue;
                    }

                  width = 10 * width + c;
                  flags |= FL_WIDTH;
                  continue;
                }

              if (c == '*')
                {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
                  if ((flags & FL_ARGNUMBER) != 0)
                    {
                      flags |= FL_ASTERISK;
                      continue;
                    }
#endif

                  if ((flags & FL_PREC) != 0)
                    {
                      prec = va_arg(ap, int);
                      if (prec < 0)
                        {
                          prec = 0;
                        }
                    }
                  else
                    {
                      width = va_arg(ap, int);
                      flags |= FL_WIDTH;

                      if (width < 0)
                        {
                          width = -width;
                          flags |= FL_LPAD;
                        }
                    }

                  continue;
                }

              if (c == '.')
                {
                  if ((flags & FL_PREC) != 0)
                    {
                      goto ret;
                    }

                  flags |= FL_PREC;
                  continue;
                }
            }

          /* Note: On NuttX, ptrdiff_t == intptr_t == ssize_t. */

          if (c == 'z' || c == 't')
            {
              switch (sizeof(size_t))
                {
                  /* The only known cases that the default will be hit are
                   * (1) the eZ80 which has sizeof(size_t) = 3 which is the
                   * same as the sizeof(int).  And (2) if
                   * CONFIG_LIBC_LONG_LONG
                   * is not enabled and sizeof(size_t) is equal to
                   * sizeof(unsigned long long).  This latter case is an
                   * error.
                   */

                  default:
                    continue;  /* Treat as integer with no size qualifier. */

                  case sizeof(unsigned short):
                    c = 'h';
                    break;

                  case sizeof(unsigned long):
                    c = 'l';
                    break;

#if defined(CONFIG_LIBC_LONG_LONG) && ULLONG_MAX != ULONG_MAX
                  case sizeof(unsigned long long):
                    c = 'l';
                    flags |= FL_LONG;
                    flags &= ~FL_SHORT;
                    break;
#endif
                }
            }

          if (c == 'j')
            {
              /* Same as long long if available. Otherwise, long. */

#ifdef CONFIG_LIBC_LONG_LONG
              flags |= FL_REPD_TYPE;
#endif
              flags |= FL_LONG;
              flags &= ~FL_SHORT;
              continue;
            }

          if (c == 'l')
            {
              if ((flags & FL_LONG) != 0)
                {
                  flags |= FL_REPD_TYPE;
                }

              flags |= FL_LONG;
              flags &= ~FL_SHORT;
              continue;
            }

          if (c == 'h')
            {
              if ((flags & FL_SHORT) != 0)
                {
                  flags |= FL_REPD_TYPE;
                }

              flags |= FL_SHORT;
              flags &= ~FL_LONG;
              continue;
            }

          break;
        }
      while ((c = fmt_char(fmt)) != 0);

      /* Only a format character is valid.  */

#if 'F' != 'E'+1  ||  'G' != 'F'+1  ||  'f' != 'e'+1  ||  'g' != 'f'+1
#  error
#endif

      if (c == 'p')
        {
          if (fmt_char(fmt) == 'V')
            {
              struct va_format *vaf = va_arg(ap, void *);
#ifdef va_copy
              va_list copy;

              va_copy(copy, *vaf->va);
              early_vprintf(vaf->fmt, copy);
              va_end(copy);
#else
              k_vprintf(vaf->fmt, *vaf->va);
#endif
              continue;
            }
          else
            {
              fmt_ungetc(fmt);
            }

          /* Determine size of pointer and set flags accordingly */

          flags &= ~(FL_LONG | FL_REPD_TYPE);

#ifdef CONFIG_LIBC_LONG_LONG
          if (sizeof(void *) == sizeof(unsigned long long))
            {
              flags |= (FL_LONG | FL_REPD_TYPE);
            }
          else
#endif
          if (sizeof(void *) == sizeof(unsigned long))
            {
              flags |= FL_LONG;
            }
        }

#ifdef CONFIG_LIBC_NUMBERED_ARGS

      if ((flags & FL_ARGNUMBER) != 0)
        {
          if (argnumber > 0 && argnumber <= numargs)
            {
              if (stream == NULL)
                {
                  if ((c >= 'E' && c <= 'G')
                      || (c >= 'e' && c <= 'g'))
                    {
                      arglist[argnumber - 1].type = TYPE_DOUBLE;
                    }
                  else if (c == 'i' || c == 'd' || c == 'u' || c == 'p')
                    {
                      if ((flags & FL_LONG) == 0)
                        {
                          arglist[argnumber - 1].type = TYPE_INT;
                        }
                      else if ((flags & FL_REPD_TYPE) == 0)
                        {
                          arglist[argnumber - 1].type = TYPE_LONG;
                        }
                      else
                        {
                          arglist[argnumber - 1].type = TYPE_LONG_LONG;
                        }
                    }
                  else if (c == 'c')
                    {
                      arglist[argnumber - 1].type = TYPE_INT;
                    }
                  else if (c == 's')
                    {
                      arglist[argnumber - 1].type = TYPE_CHAR_POINTER;
                    }

                  if (argnumber > total_len)
                    {
                      total_len = argnumber;
                    }

                  continue; /* We do only parsing */
                }
            }
          else
            {
              goto ret;
            }
        }
      else if (stream == NULL)
        {
          continue; /* We do only parsing */
        }

#endif

#ifdef CONFIG_LIBC_FLOATINGPOINT
      if (c >= 'E' && c <= 'G')
        {
          flags |= FL_FLTUPP;
          c += 'e' - 'E';
          goto flt_oper;
        }
      else if (c >= 'e' && c <= 'g')
        {
          double value;
          int exp;              /* Exponent of master decimal digit */
          int n;
          uint8_t sign;         /* Sign character (or 0) */
          uint8_t ndigs;        /* Number of digits to convert */
          uint8_t ndecimal;     /* Digits after decimal (for 'f' format), 0 if
                                 * no limit */

          flags &= ~FL_FLTUPP;

          flt_oper:
          ndigs = 0;
          if ((flags & FL_PREC) == 0)
            {
              prec = 6;
            }

          flags &= ~(FL_FLTEXP | FL_FLTFIX);

          if (c == 'e')
            {
              ndigs = prec + 1;
              ndecimal = 0;
              flags |= FL_FLTEXP;
            }
          else if (c == 'f')
            {
              ndigs = DTOA_MAX_DIG;
              ndecimal = prec;
              flags |= FL_FLTFIX;
            }
          else
            {
              ndigs = prec;
              ndecimal = 0;
            }

          if (ndigs > DTOA_MAX_DIG)
            {
              ndigs = DTOA_MAX_DIG;
            }

#ifdef CONFIG_LIBC_NUMBERED_ARGS
          if ((flags & FL_ARGNUMBER) != 0)
            {
              value = arglist[argnumber - 1].value.d;
            }
          else
            {
              value = va_arg(ap, double);
            }
#else
          value = va_arg(ap, double);
#endif

          ndigs = early__dtoa_engine(value, &_dtoa, ndigs,
                                ndecimal);
          exp = _dtoa.exp;

          sign = 0;
          if ((_dtoa.flags & DTOA_MINUS) && !(_dtoa.flags & DTOA_NAN))
            {
              sign = '-';
            }
          else if ((flags & FL_PLUS) != 0)
            {
              sign = '+';
            }
          else if ((flags & FL_SPACE) != 0)
            {
              sign = ' ';
            }

          if (_dtoa.flags & (DTOA_NAN | DTOA_INF))
            {
              const char *p;

              ndigs = sign ? 4 : 3;
              if (width > ndigs)
                {
                  width -= ndigs;
                  if ((flags & FL_LPAD) == 0)
                    {
                      do
                        {
                          early_putchar(' ');
                        }
                      while (--width);
                    }
                }
              else
                {
                  width = 0;
                }

              if (sign)
                {
  
                  early_putchar(sign);
                }

              p = "inf";
              if (_dtoa.flags & DTOA_NAN)
                {
                  p = "nan";
                }

#  if ('I'-'i' != 'N'-'n') || ('I'-'i' != 'F'-'f') || ('I'-'i' != 'A'-'a')
#    error
#  endif
              while ((ndigs = *p) != 0)
                {
                  if ((flags & FL_FLTUPP) != 0)
                    {
                      ndigs += 'I' - 'i';
                    }

                  early_putchar(ndigs);
                  p++;
                }

              goto tail;
            }

          if ((flags & (FL_FLTEXP | FL_FLTFIX)) == 0)
            {
              /* 'g(G)' format */

              prec = ndigs;

              /* Remove trailing zeros */

              while (ndigs > 0 && _dtoa.digits[ndigs - 1] == '0')
                {
                  ndigs--;
                }

              if (-4 <= exp && exp < prec)
                {
                  flags |= FL_FLTFIX;

                  if (exp < 0 || ndigs > exp)
                    {
                      prec = ndigs - (exp + 1);
                    }
                  else
                    {
                      prec = 0;
                    }
                }
              else
                {
                  /* Limit displayed precision to available precision */

                  prec = ndigs - 1;
                }
            }

          /* Conversion result length, width := free space length */

          if ((flags & FL_FLTFIX) != 0)
            {
              n = (exp > 0 ? exp + 1 : 1);
            }
          else
            {
              n = 5;              /* 1e+00 */
            }

          if (sign != 0)
            {
              n += 1;
            }

          if (prec != 0)
            {
              n += prec + 1;
            }
          else if ((flags & FL_ALT) != 0)
            {
              n += 1;
            }

          width = width > n ? width - n : 0;

          /* Output before first digit */

          if ((flags & (FL_LPAD | FL_ZFILL)) == 0)
            {
              while (width)
                {
                  early_putchar(' ');
                  width--;
                }
            }

          if (sign != 0)
            {
              early_putchar(sign);
            }

          if ((flags & FL_LPAD) == 0)
            {
              while (width)
                {
                  early_putchar('0');
                  width--;
                }
            }

          if ((flags & FL_FLTFIX) != 0)
            {
              /* 'f' format */

              char out;

              /* At this point, we should have exp exponent of leftmost digit
               * in _dtoa.digits ndigs number of buffer digits to print prec
               * number of digits after decimal In the loop, 'n' walks over
               * the exponent value
               */

              n = exp > 0 ? exp : 0;    /* Exponent of left digit */
              do
                {
                  /* Insert decimal point at correct place */

                  if (n == -1)
                    {
                      early_putchar('.');
                    }

                  /* Pull digits from buffer when in-range, otherwise use 0 */

                  if (0 <= exp - n && exp - n < ndigs)
                    {
                      out = _dtoa.digits[exp - n];
                    }
                  else
                    {
                      out = '0';
                    }

                  if (--n < -prec)
                    {
                      if ((flags & FL_ALT) != 0 && n == -1)
                        {
                          early_putchar('.');
                        }

                      break;
                    }

                  early_putchar(out);
                }
              while (1);

              if (n == exp && (_dtoa.digits[0] > '5' ||
                  (_dtoa.digits[0] == '5' && !(_dtoa.flags & DTOA_CARRY))))
                {
                  out = '1';
                }

              early_putchar(out);
            }
          else
            {
              /* 'e(E)' format
               *
               * Mantissa
               */

              if (_dtoa.digits[0] != '1')
                {
                  _dtoa.flags &= ~DTOA_CARRY;
                }

              early_putchar(_dtoa.digits[0]);
              if (prec > 0)
                {
                  uint8_t pos;
                  early_putchar('.');
                  for (pos = 1; pos < 1 + prec; pos++)
                    {
                      early_putchar(pos < ndigs ? _dtoa.digits[pos] : '0');
                    }
                }
              else if ((flags & FL_ALT) != 0)
                {
                  early_putchar('.');
                }

              /* Exponent */
              early_putchar(flags & FL_FLTUPP ? 'E' : 'e');
              ndigs = '+';
              if (exp < 0 || (exp == 0 && (_dtoa.flags & DTOA_CARRY) != 0))
                {
                  exp = -exp;
                  ndigs = '-';
                }

              early_putchar(ndigs);
              for (ndigs = '0'; exp >= 10; exp -= 10)
                {
                  ndigs += 1;
                }

              early_putchar(ndigs);
              early_putchar('0' + exp);
            }

          goto tail;
        }

#else /* !CONFIG_LIBC_FLOATINGPOINT */
      if ((c >= 'E' && c <= 'G') || (c >= 'e' && c <= 'g'))
        {
          va_arg(ap, double);
          pnt  = "*float*";
          size = sizeof("*float*") - 1;
          goto str_lpad;
        }
#endif

      switch (c)
        {
        case 'c':
#ifdef CONFIG_LIBC_NUMBERED_ARGS
          if ((flags & FL_ARGNUMBER) != 0)
            {
              buf[0] = (int)arglist[argnumber - 1].value.u;
            }
          else
            {
              buf[0] = va_arg(ap, int);
            }
#else
          buf[0] = va_arg(ap, int);
#endif
          pnt = (char *) buf;
          size = 1;
          goto str_lpad;

        case 's':
        case 'S':
#ifdef CONFIG_LIBC_NUMBERED_ARGS
          if ((flags & FL_ARGNUMBER) != 0)
            {
              pnt = (char *)arglist[argnumber - 1].value.cp;
            }
          else
            {
              pnt = va_arg(ap, char *);
            }
#else
          pnt = va_arg(ap, char *);
#endif
          if (pnt == NULL)
            {
              pnt = g_nullstring;
            }

          size = early_strnlen(pnt, (flags & FL_PREC) ? prec : ~0);

        str_lpad:
          if ((flags & FL_LPAD) == 0)
            {
              while (size < width)
                {
                  early_putchar(' ');
                  width--;
                }
            }

          while (size)
            {
              early_putchar(*pnt++);
              if (width != 0)
                {
                  width -= 1;
                }

              size -= 1;
            }

          goto tail;
        }

      if (c == 'd' || c == 'i')
        {
#ifndef CONFIG_LIBC_LONG_LONG
          long x;
#else
          long long x;

          if ((flags & FL_LONG) != 0 && (flags & FL_REPD_TYPE) != 0)
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = (long long)arglist[argnumber - 1].value.ull;
                }
              else
                {
                  x = va_arg(ap, long long);
                }
#else
                x = va_arg(ap, long long);
#endif
            }
          else
#endif
          if ((flags & FL_LONG) != 0)
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = (long)arglist[argnumber - 1].value.ul;
                }
              else
                {
                  x = va_arg(ap, long);
                }
#else
                x = va_arg(ap, long);
#endif
            }
          else
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = (int)arglist[argnumber - 1].value.u;
                }
              else
                {
                  x = va_arg(ap, int);
                }
#else
                x = va_arg(ap, int);
#endif
              if ((flags & FL_SHORT) != 0)
                {
                  if ((flags & FL_REPD_TYPE) == 0)
                    {
                      x = (short)x;
                    }
                  else
                    {
                      x = (signed char)x;
                    }
                }
            }

          flags &= ~(FL_NEGATIVE | FL_ALT);
          if (x < 0)
            {
              x = -x;
              flags |= FL_NEGATIVE;
            }

          if ((flags & FL_PREC) != 0 && prec == 0 && x == 0)
            {
              c = 0;
            }
          else
            {
              c = early__ultoa_invert(x, (char *)buf, 10) - (char *)buf;
            }
        }
      else
        {
          int base;
#ifndef CONFIG_LIBC_LONG_LONG
          unsigned long x;
#else
          unsigned long long x;

          if ((flags & FL_LONG) != 0 && (flags & FL_REPD_TYPE) != 0)
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = arglist[argnumber - 1].value.ull;
                }
              else
                {
                  x = va_arg(ap, unsigned long long);
                }
#else
                x = va_arg(ap, unsigned long long);
#endif
            }
          else
#endif
          if ((flags & FL_LONG) != 0)
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = arglist[argnumber - 1].value.ul;
                }
              else
                {
                  x = va_arg(ap, unsigned long);
                }
#else
                x = va_arg(ap, unsigned long);
#endif
            }
          else
            {
#ifdef CONFIG_LIBC_NUMBERED_ARGS
              if ((flags & FL_ARGNUMBER) != 0)
                {
                  x = (unsigned int)arglist[argnumber - 1].value.u;
                }
              else
                {
                  x = va_arg(ap, unsigned int);
                }
#else
                x = va_arg(ap, unsigned int);
#endif
              if ((flags & FL_SHORT) != 0)
                {
                  if ((flags & FL_REPD_TYPE) == 0)
                    {
                      x = (unsigned short)x;
                    }
                  else
                    {
                      x = (unsigned char)x;
                    }
                }
            }

          flags &= ~(FL_PLUS | FL_SPACE);

          switch (c)
            {
            case 'u':
              flags &= ~FL_ALT;
              base = 10;
              break;

            case 'o':
              base = 8;
              break;

            case 'p':
              flags |= FL_ALT;

              /* no break */

            case 'x':
              if ((flags & FL_ALT) != 0)
                {
                  flags |= FL_ALTHEX;
                }

              base = 16;
              break;

            case 'X':
              if ((flags & FL_ALT) != 0)
                {
                  flags |= (FL_ALTHEX | FL_ALTUPP);
                }

              base = 16 | XTOA_UPPER;
              break;

            default:
              early_putchar('%');
              early_putchar(c);
              continue;
            }

          if ((flags & FL_PREC) != 0 && prec == 0 && x == 0)
            {
              c = 0;
            }
          else
            {
              c = early__ultoa_invert(x, (char *)buf, base) - (char *)buf;
            }

          flags &= ~FL_NEGATIVE;
        }

      len = c;

      if ((flags & FL_PREC) != 0)
        {
          flags &= ~FL_ZFILL;
          if (len < prec)
            {
              len = prec;
              if ((flags & FL_ALT) != 0 && (flags & FL_ALTHEX) == 0)
                {
                  flags &= ~FL_ALT;
                }
            }
        }

      if ((flags & FL_ALT) != 0)
        {
          if (buf[c - 1] == '0')
            {
              flags &= ~(FL_ALT | FL_ALTHEX | FL_ALTUPP);
            }
          else
            {
              len += 1;
              if ((flags & FL_ALTHEX) != 0)
                {
                  len += 1;
                }
            }
        }
      else if ((flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) != 0)
        {
          len += 1;
        }

      if ((flags & FL_LPAD) == 0)
        {
          if ((flags & FL_ZFILL) != 0)
            {
              prec = c;
              if (len < width)
                {
                  prec += width - len;
                  len = width;
                }
            }

          while (len < width)
            {
              early_putchar(' ');
              len++;
            }
        }

      width = (len < width) ? width - len : 0;

      if ((flags & FL_ALT) != 0)
        {
          early_putchar('0');
          if ((flags & FL_ALTHEX) != 0)
            {
              early_putchar(flags & FL_ALTUPP ? 'X' : 'x');
            }
        }
      else if ((flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) != 0)
        {
          unsigned char z = ' ';
          if ((flags & FL_PLUS) != 0)
            {
              z = '+';
            }

          if ((flags & FL_NEGATIVE) != 0)
            {
              z = '-';
            }

          early_putchar(z);
        }

      while (prec > c)
        {
          early_putchar('0');
          prec--;
        }

      while (c)
        {
          early_putchar(buf[--c]);
        }

tail:

      /* Tail is possible.  */

      while (width)
        {
          early_putchar(' ');
          width--;
        }
    }

ret:

  return total_len;
}


int BOOTPHYSIC early_printf(const char *fmt, ...)
{
    va_list ap;
    int     ret;

    va_start(ap, fmt); 
    ret = early_vprintf(fmt, ap);
    va_end(ap);

    return ret;
}

void BOOTPHYSIC early_uart_init(unsigned long base)
{
    uart_base = base;
}

