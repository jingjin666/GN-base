#include <chinos/config.h>

#include <k_stdint.h>
#include <k_string.h>
#include <k_stdio.h>
#include <k_limits.h>

#include <uart.h>

#define kputchar uart_putchar

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

static const char g_nullstring[] = "(null)";

/* '__ftoa_engine' return next flags (in buf[0]):	*/
#define	FTOA_MINUS	1
#define	FTOA_ZERO	2
#define	FTOA_INF	4
#define	FTOA_NAN	8
#define	FTOA_CARRY	16	/* Carry was to master position.	*/

/* Next flags are to use with `base'. Unused fields are reserved.	*/
#define XTOA_PREFIX	0x0100	/* put prefix for octal or hex	*/
#define XTOA_UPPER	0x0200	/* use upper case letters	*/

static float fabsf(float x)
{
    return ((x < 0) ? -x : x);
}

/*
 * 2^b ~= f * r * 10^e
 * where
 * i = b div 8
 * r = 2^(b mod 8)
 * f = factorTable[i]
 * e = exponentTable[i]
 */
static const int8_t exponentTable[32] = {
    -36, -33, -31, -29, -26, -24, -21, -19,
    -17, -14, -12, -9,  -7, -4, -2,  0,
    3, 5, 8, 10, 12, 15,  17, 20,
    22, 24, 27, 29,  32, 34, 36, 39
};

static const uint32_t factorTable[32] = {
    2295887404UL,
    587747175UL,
    1504632769UL,
    3851859889UL,
    986076132UL,
    2524354897UL,
    646234854UL,
    1654361225UL,
    4235164736UL,
    1084202172UL,
    2775557562UL,
    710542736UL,
    1818989404UL,
    465661287UL,
    1192092896UL,
    3051757813UL,
    781250000UL,
    2000000000UL,
    512000000UL,
    1310720000UL,
    3355443200UL,
    858993459UL,
    2199023256UL,
    562949953UL,
    1441151881UL,
    3689348815UL,
    944473297UL,
    2417851639UL,
    618970020UL,
    1584563250UL,
    4056481921UL,
    1038459372UL
};

static int16_t ftoa_engine(float val, char *buf, uint8_t precision, uint8_t maxDecimals) 
{
    uint8_t flags;

    // Bit reinterpretation hacks. This will ONLY work on little endian machines.
    uint8_t *valbits = (uint8_t*)&val;
    union {
        float v;
        uint32_t u;
    } x;
    x.v = val;
    uint32_t frac = x.u & 0x007fffffUL;

    if (precision>7) precision=7;

    // Read the sign, shift the exponent in place and delete it from frac.
    if (valbits[3] & (1<<7)) flags = FTOA_MINUS; else flags = 0;
    uint8_t exp = valbits[3]<<1;
    if(valbits[2] & (1<<7)) exp++;    // TODO possible but in case of subnormal

    // Test for easy cases, zero and NaN
    if(exp==0 && frac==0) {
        buf[0] = flags | FTOA_ZERO;
        uint8_t i;
        for(i=0; i<=precision; i++) {
            buf[i+1] = '0';
        }
        return 0;
    }

    if(exp == 0xff) {
        if(frac == 0) flags |= FTOA_INF; else flags |= FTOA_NAN;
    }

    // The implicit leading 1 is made explicit, except if value subnormal.
    if (exp != 0) frac |= (1UL<<23);

    uint8_t idx = exp>>3;
    int8_t exp10 = exponentTable[idx];

    // We COULD try making the multiplication in situ, where we make
    // frac and a 64 bit int overlap in memory and select/weigh the
    // upper 32 bits that way. For starters, this is less risky:
    int64_t prod = (int64_t)frac * (int64_t)factorTable[idx];

    // The expConvFactorTable are factor are correct iff the lower 3 exponent
    // bits are 1 (=7). Else we need to compensate by divding frac.
    // If the lower 3 bits are 7 we are right.
    // If the lower 3 bits are 6 we right-shift once
    // ..
    // If the lower 3 bits are 0 we right-shift 7x
    prod >>= (15-(exp & 7));

    // Now convert to decimal.
    uint8_t hadNonzeroDigit = 0; // a flag
    uint8_t outputIdx = 0;
    int64_t decimal = 100000000000000ull;

    do {
        char digit = '0';
        while(1) {// find the first nonzero digit or any of the next digits.
            while ((prod -= decimal) >= 0)
                digit++;
            // Now we got too low. Fix it by adding again, once.
            // it might appear more efficient to check before subtract, or
            // to save and restore last nonnegative value - but in fact
            // they take as long time and more space.
            prod += decimal;
            decimal /= 10;

            // If already found a leading nonzero digit, accept zeros.
            if (hadNonzeroDigit) break;

            // Else, don't return results with a leading zero! Instead
            // skip those and decrement exp10 accordingly.
            if(digit == '0') {
                exp10--;
                continue;
            }

            hadNonzeroDigit = 1;

            // Compute how many digits N to output.
            if(maxDecimals != 0) {                        // If limiting decimals...
                int8_t beforeDP = exp10+1;                // Digits before point
                if (beforeDP < 1) beforeDP = 1;            // Numbers < 1 should also output at least 1 digit.
                /*
                 * Below a simpler version of this:
                int8_t afterDP = outputNum - beforeDP;
                if (afterDP > maxDecimals-1)
                    afterDP = maxDecimals-1;
                outputNum = beforeDP + afterDP;
                */
                maxDecimals = maxDecimals+beforeDP-1;
                if (precision > maxDecimals)
                    precision = maxDecimals;

            } else {
                precision++;                            // Output one more digit than the param value.
            }

            break;
        }

        // Now have a digit.
        outputIdx++;
        if(digit < '0' + 10) // normal case.
            buf[outputIdx] = digit;
        else {
            // Abnormal case, write 9s and bail.
            // We might as well abuse hadNonzeroDigit as counter, it will not be used again.
            for(hadNonzeroDigit=outputIdx; hadNonzeroDigit>0; hadNonzeroDigit--)
                buf[hadNonzeroDigit] = '9';
            goto roundup; // this is ugly but it _is_ code derived from assembler :)
        }
    } while (outputIdx<precision);

    // Rounding:
    decimal *= 10;

    if (prod - (decimal >> 1) >= 0) {

    roundup:
        // Increment digit, cascade
        while(outputIdx != 0) {
            if(++buf[outputIdx] == '0' + 10) {
                if(outputIdx == 1) {
                    buf[outputIdx] = '1';
                    exp10++;
                    flags |= FTOA_CARRY;
                    break;
                } else
                    buf[outputIdx--] = '0'; // and the loop continues, carrying to next digit.
            }
            else break;
        }
    }

    buf[0] = flags;
    return exp10;
}

#ifdef CONFIG_LIBC_LONG_LONG
char *__ultoa_invert(unsigned long long val, char *str, int base)
#else
char *__ultoa_invert(unsigned long val, char *str, int base)
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

static char * ulltoa_invert (uint64_t val, char *s, uint8_t base)
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

static int k_vprintf(const char *fmt, va_list ap)
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
              kputchar(c);
            }
#else
          kputchar(c);
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
              k_vprintf(vaf->fmt, copy);
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

          ndigs = __dtoa_engine(value, &_dtoa, ndigs,
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
                          kputchar(' ');
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
  
                  kputchar(sign);
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

                  kputchar(ndigs);
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
                  kputchar(' ');
                  width--;
                }
            }

          if (sign != 0)
            {
              kputchar(sign);
            }

          if ((flags & FL_LPAD) == 0)
            {
              while (width)
                {
                  kputchar('0');
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
                      kputchar('.');
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
                          kputchar('.');
                        }

                      break;
                    }

                  kputchar(out);
                }
              while (1);

              if (n == exp && (_dtoa.digits[0] > '5' ||
                  (_dtoa.digits[0] == '5' && !(_dtoa.flags & DTOA_CARRY))))
                {
                  out = '1';
                }

              kputchar(out);
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

              kputchar(_dtoa.digits[0]);
              if (prec > 0)
                {
                  uint8_t pos;
                  kputchar('.');
                  for (pos = 1; pos < 1 + prec; pos++)
                    {
                      kputchar(pos < ndigs ? _dtoa.digits[pos] : '0');
                    }
                }
              else if ((flags & FL_ALT) != 0)
                {
                  kputchar('.');
                }

              /* Exponent */
              kputchar(flags & FL_FLTUPP ? 'E' : 'e');
              ndigs = '+';
              if (exp < 0 || (exp == 0 && (_dtoa.flags & DTOA_CARRY) != 0))
                {
                  exp = -exp;
                  ndigs = '-';
                }

              kputchar(ndigs);
              for (ndigs = '0'; exp >= 10; exp -= 10)
                {
                  ndigs += 1;
                }

              kputchar(ndigs);
              kputchar('0' + exp);
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

          size = k_strnlen(pnt, (flags & FL_PREC) ? prec : ~0);

        str_lpad:
          if ((flags & FL_LPAD) == 0)
            {
              while (size < width)
                {
                  kputchar(' ');
                  width--;
                }
            }

          while (size)
            {
              kputchar(*pnt++);
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
              c = __ultoa_invert(x, (char *)buf, 10) - (char *)buf;
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
              kputchar('%');
              kputchar(c);
              continue;
            }

          if ((flags & FL_PREC) != 0 && prec == 0 && x == 0)
            {
              c = 0;
            }
          else
            {
              c = __ultoa_invert(x, (char *)buf, base) - (char *)buf;
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
              kputchar(' ');
              len++;
            }
        }

      width = (len < width) ? width - len : 0;

      if ((flags & FL_ALT) != 0)
        {
          kputchar('0');
          if ((flags & FL_ALTHEX) != 0)
            {
              kputchar(flags & FL_ALTUPP ? 'X' : 'x');
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

          kputchar(z);
        }

      while (prec > c)
        {
          kputchar('0');
          prec--;
        }

      while (c)
        {
          kputchar(buf[--c]);
        }

tail:

      /* Tail is possible.  */

      while (width)
        {
          kputchar(' ');
          width--;
        }
    }

ret:

  return total_len;
}


static void print_prefix(void)
{
    kputchar('[');
    kputchar('K');
    kputchar(']');
    kputchar(' ');
}

void _kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    k_vprintf(fmt, ap);
    va_end(ap);
}

void kprintf(const char *fmt, ...)
{
    print_prefix();
    va_list ap;
    va_start(ap, fmt);
    k_vprintf(fmt, ap);
    va_end(ap);
}

void kputs(const char *s)
{
    print_prefix();
    while (*s != '\0') {
        kputchar(*s);
        s++;
    };
    
    kputchar('\r');
    kputchar('\n');
}

void k_puts(const char *s)
{
    while (*s != '\0') {
        kputchar(*s);
        s++;
    };
}

