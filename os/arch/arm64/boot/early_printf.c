#include <chinos/config.h>

#include <kernel.h>
#include <k_stdarg.h>
#include <k_types.h>

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

static BOOTPHYSIC void *early_memset(void *s, int c, size_t n)
{
#ifdef CONFIG_MEMSET_OPTSPEED
  /* This version is optimized for speed (you could do better
   * still by exploiting processor caching or memory burst
   * knowledge.)
   */

  uintptr_t addr  = (uintptr_t)s;
  uint16_t  val16 = ((uint16_t)c << 8) | (uint16_t)c;
  uint32_t  val32 = ((uint32_t)val16 << 16) | (uint32_t)val16;
#ifdef CONFIG_MEMSET_64BIT
  uint64_t  val64 = ((uint64_t)val32 << 32) | (uint64_t)val32;
#endif

  /* Make sure that there is something to be cleared */

  if (n > 0)
    {
      /* Align to a 16-bit boundary */

      if ((addr & 1) != 0)
        {
          *(uint8_t *)addr = (uint8_t)c;
          addr += 1;
          n    -= 1;
        }

      /* Check if there are at least 16-bits left to be written */

      if (n >= 2)
        {
          /* Align to a 32-bit boundary (we know that the destination
           * address is already aligned to at least a 16-bit boundary).
           */

          if ((addr & 3) != 0)
            {
              *(uint16_t *)addr = val16;
              addr += 2;
              n    -= 2;
            }

#ifndef CONFIG_MEMSET_64BIT
          /* Loop while there are at least 32-bits left to be written */

          while (n >= 4)
            {
              *(uint32_t *)addr = val32;
              addr += 4;
              n    -= 4;
            }
#else
          /* Check if there are at least 32-bits left to be written */

          if (n >= 4)
            {
              /* Align to a 64-bit boundary (we know that the destination
               * address is already aligned to at least a 32-bit boundary).
               */

              if ((addr & 7) != 0)
                {
                  *(uint32_t *)addr = val32;
                  addr += 4;
                  n    -= 4;
                }

              /* Loop while there are at least 64-bits left to be written */

              while (n >= 8)
                {
                  *(uint64_t *)addr = val64;
                  addr += 8;
                  n    -= 8;
                }
            }
#endif
        }

#ifdef CONFIG_MEMSET_64BIT
      /* We may get here with n in the range 0..7.  If n >= 4, then we should
       * have 64-bit alignment.
       */

      if (n >= 4)
        {
          *(uint32_t *)addr = val32;
          addr += 4;
          n    -= 4;
        }
#endif

      /* We may get here under the following conditions:
       *
       *   n = 0, addr may or may not be aligned
       *   n = 1, addr is aligned to at least a 16-bit boundary
       *   n = 2, addr is aligned to a 32-bit boundary
       *   n = 3, addr is aligned to a 32-bit boundary
       */

      if (n >= 2)
        {
          *(uint16_t *)addr = val16;
          addr += 2;
          n    -= 2;
        }

      if (n >= 1)
        {
          *(uint8_t *)addr = (uint8_t)c;
        }
    }
#else
  /* This version is optimized for size */

  unsigned char *p = (unsigned char *)s;
  while (n-- > 0) *p++ = c;
#endif
  return s;
}

static BOOTPHYSIC float fabsf(float x)
{
    return ((x < 0) ? -x : x);
}

#define FL_ZFILL    0x01
#define FL_PLUS     0x02
#define FL_SPACE    0x04
#define FL_LPAD     0x08
#define FL_ALT      0x10
#define FL_WIDTH    0x20
#define FL_PREC     0x40
#define FL_LONG     0x80
#define FL_LONGLONG 0x100

#define FL_NEGATIVE FL_LONG

#define FL_ALTUPP   FL_PLUS
#define FL_ALTHEX   FL_SPACE

#define FL_FLTUPP   FL_ALT
#define FL_FLTEXP   FL_PREC
#define FL_FLTFIX   FL_LONG

/* '__ftoa_engine' return next flags (in buf[0]):	*/
#define	FTOA_MINUS	1
#define	FTOA_ZERO	2
#define	FTOA_INF	4
#define	FTOA_NAN	8
#define	FTOA_CARRY	16	/* Carry was to master position.	*/

/* Next flags are to use with `base'. Unused fields are reserved.	*/
#define XTOA_PREFIX	0x0100	/* put prefix for octal or hex	*/
#define XTOA_UPPER	0x0200	/* use upper case letters	*/

/*
 * 2^b ~= f * r * 10^e
 * where
 * i = b div 8
 * r = 2^(b mod 8)
 * f = factorTable[i]
 * e = exponentTable[i]
 */
 static BOOTDATA int8_t exponentTable[32] = {
    -36, -33, -31, -29, -26, -24, -21, -19,
    -17, -14, -12, -9,  -7, -4, -2,  0,
    3, 5, 8, 10, 12, 15,  17, 20,
    22, 24, 27, 29,  32, 34, 36, 39
};

static BOOTDATA uint32_t factorTable[32] = {
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

static BOOTPHYSIC int16_t ftoa_engine(float val, char *buf, uint8_t precision, uint8_t maxDecimals) 
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

static BOOTPHYSIC char * ultoa_invert (uint32_t val, char *s, uint8_t base)
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

	// 33 bits would have been enough.
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

static BOOTPHYSIC char * ulltoa_invert (uint64_t val, char *s, uint8_t base)
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
    unsigned char c;        /* holds a char from the format string */
    uint16_t flags;
    unsigned char width;
    unsigned char prec;
    unsigned char buf[23];
    float value;

    for (;;) {
        /*
         * Process non-format characters
         */
        for (;;) {
            c = *fmt++;
            if (!c) {
                return 0;
            }
            if (c == '%') {
                c = *fmt++;
                if (c != '%') {
                    break;
                }
            }
            /* emit cr before lf to make most terminals happy */
            if (c == '\n') {
                early_putchar('\r');
            }
            early_putchar(c);
        }

        flags = 0;
        width = 0;
        prec = 0;

        /*
         * Process format adjustment characters, precision, width.
         */
        do {
            if (flags < FL_WIDTH) {
                switch (c) {
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

            if (flags < FL_LONG) {
                if (c >= '0' && c <= '9') {
                    c -= '0';
                    if (flags & FL_PREC) {
                        prec = 10*prec + c;
                        continue;
                    }
                    width = 10*width + c;
                    flags |= FL_WIDTH;
                    continue;
                }
                if (c == '.') {
                    if (flags & FL_PREC) {
                        return 0;
                    }
                    flags |= FL_PREC;
                    continue;
                }
                if (c == 'l') {
                    flags |= FL_LONG;
                    continue;
                }
                if (c == 'h') {
                    continue;
                }
            } else if ((flags & FL_LONG) && c == 'l') {
                flags |= FL_LONGLONG;
                continue;
            }

            break;
        } while ((c = *fmt++) != 0);

        /*
         * Handle floating-point formats E, F, G, e, f, g.
         */
        if (c >= 'E' && c <= 'G') {
            flags |= FL_FLTUPP;
            c += 'e' - 'E';
            goto flt_oper;
        } else if (c >= 'e' && c <= 'g') {
            int exp;                /* exponent of master decimal digit     */
            int n;
            unsigned char vtype;    /* result of float value parse  */
            unsigned char sign;     /* sign character (or 0)        */
            unsigned char ndigs;

            flags &= ~FL_FLTUPP;

flt_oper:
            value = va_arg(ap,double);

            if (!(flags & FL_PREC)) {
                prec = 6;
            }
            flags &= ~(FL_FLTEXP | FL_FLTFIX);
            if (c == 'e') {
                flags |= FL_FLTEXP;
            } else if (c == 'f') {
                flags |= FL_FLTFIX;
            } else if (prec > 0) {
                prec -= 1;
            }

            if ((flags & FL_FLTFIX) && fabsf(value) > 9999999) {
                flags = (flags & ~FL_FLTFIX) | FL_FLTEXP;
            }

            if (flags & FL_FLTFIX) {
                vtype = 7;              /* 'prec' arg for 'ftoa_engine' */
                ndigs = prec < 60 ? prec + 1 : 60;
            } else {
                if (prec > 10) {
                    prec = 10;
                }
                vtype = prec;
                ndigs = 0;
            }
            early_memset(buf, 0, sizeof(buf));
            exp = ftoa_engine(value, (char *)buf, vtype, ndigs);
            vtype = buf[0];

            sign = 0;
            if ((vtype & FTOA_MINUS) && !(vtype & FTOA_NAN))
                sign = '-';
            else if (flags & FL_PLUS)
                sign = '+';
            else if (flags & FL_SPACE)
                sign = ' ';

            if (vtype & (FTOA_NAN | FTOA_INF)) {
                ndigs = sign ? 4 : 3;
                if (width > ndigs) {
                    width -= ndigs;
                    if (!(flags & FL_LPAD)) {
                        do {
                            early_putchar(' ');
                        } while (--width);
                    }
                } else {
                    width = 0;
                }
                if (sign) {
                    early_putchar(sign);
                }

                const char *p = "inf";
                if (vtype & FTOA_NAN)
                    p = "nan";
                while ((ndigs = *p) != 0) {
                    if (flags & FL_FLTUPP)
                        ndigs += 'I' - 'i';
                    early_putchar(ndigs);
                    p++;
                }
                goto tail;
            }

            /* Output format adjustment, number of decimal digits in buf[] */
            if (flags & FL_FLTFIX) {
                ndigs += exp;
                if ((vtype & FTOA_CARRY) && buf[1] == '1') {
                    ndigs -= 1;
                }
                if ((signed char)ndigs < 1) {
                    ndigs = 1;
                } else if (ndigs > 8) {
                    ndigs = 8;
                }
            } else if (!(flags & FL_FLTEXP)) {              /* 'g(G)' format */
                if (exp <= prec && exp >= -4) {
                    flags |= FL_FLTFIX;
                }
                while (prec && buf[1+prec] == '0') {
                    prec--;
                }
                if (flags & FL_FLTFIX) {
                    ndigs = prec + 1;               /* number of digits in buf */
                    prec = prec > exp ? prec - exp : 0;       /* fractional part length  */
                }
            }

            /* Conversion result length, width := free space length */
            if (flags & FL_FLTFIX) {
                n = (exp>0 ? exp+1 : 1);
            } else {
                n = 5;          /* 1e+00 */
            }
            if (sign) {
                n += 1;
            }
            if (prec) {
                n += prec + 1;
            }
            width = width > n ? width - n : 0;

            /* Output before first digit    */
            if (!(flags & (FL_LPAD | FL_ZFILL))) {
                while (width) {
                    early_putchar(' ');
                    width--;
                }
            }
            if (sign) {
                early_putchar(sign);
            }
            if (!(flags & FL_LPAD)) {
                while (width) {
                    early_putchar('0');
                    width--;
                }
            }

            if (flags & FL_FLTFIX) {                /* 'f' format           */

                n = exp > 0 ? exp : 0;          /* exponent of left digit */
                unsigned char v = 0;
                do {
                    if (n == -1) {
                        early_putchar('.');
                    }
                    v = (n <= exp && n > exp - ndigs)
                        ? buf[exp - n + 1] : '0';
                    if (--n < -prec || v == 0) {
                        break;
                    }
                    early_putchar(v);
                } while (1);
                if (n == exp
                    && (buf[1] > '5'
                        || (buf[1] == '5' && !(vtype & FTOA_CARRY)))) {
                    v = '1';
                }
                if (v) {
                    early_putchar(v);
                }
            } else {                                /* 'e(E)' format        */
                /* mantissa     */
                if (buf[1] != '1')
                    vtype &= ~FTOA_CARRY;
                early_putchar(buf[1]);
                if (prec) {
                    early_putchar('.');
                    sign = 2;
                    do {
                        early_putchar(buf[sign++]);
                    } while (--prec);
                }

                /* exponent     */
                early_putchar(flags & FL_FLTUPP ? 'E' : 'e');
                ndigs = '+';
                if (exp < 0 || (exp == 0 && (vtype & FTOA_CARRY) != 0)) {
                    exp = -exp;
                    ndigs = '-';
                }
                early_putchar(ndigs);
                for (ndigs = '0'; exp >= 10; exp -= 10)
                    ndigs += 1;
                early_putchar(ndigs);
                early_putchar('0' + exp);
            }

            goto tail;
        }

        /*
         * Handle string formats c, s, S.
         */
        {
            const char * pnt;
            size_t size;

            switch (c) {
            case 'c':
                buf[0] = va_arg (ap, int);
                pnt = (char *)buf;
                size = 1;
                break;

            case 's':
                pnt = va_arg (ap, char *);
                size = early_strnlen (pnt, (flags & FL_PREC) ? prec : ~0);
                break;

            default:
                    goto non_string;
            }

            if (!(flags & FL_LPAD)) {
                while (size < width) {
                    early_putchar(' ');
                    width--;
                }
            }

            while (size) {
                early_putchar(*pnt++);
                if (width) width -= 1;
                size -= 1;
            }
            goto tail;
        }

    non_string:
        
        /*
         * Handle integer formats variations for d/i, u, o, p, x, X.
         */
        if (c == 'd' || c == 'i') {
            if (flags & FL_LONGLONG) {
                int64_t x = va_arg(ap,long long);
                flags &= ~(FL_NEGATIVE | FL_ALT);
                if (x < 0) {
                    x = -x;
                    flags |= FL_NEGATIVE;
                }
                c = ulltoa_invert (x, (char *)buf, 10) - (char *)buf;
            } else {
                long x = (flags & FL_LONG) ? va_arg(ap,long) : va_arg(ap,int);
                flags &= ~(FL_NEGATIVE | FL_ALT);
                if (x < 0) {
                    x = -x;
                    flags |= FL_NEGATIVE;
                }
                c = ultoa_invert (x, (char *)buf, 10) - (char *)buf;
            }
        } else {
            int base;

            if (c == 'u') {
                flags &= ~FL_ALT;
                base = 10;
                goto ultoa;
            }

            flags &= ~(FL_PLUS | FL_SPACE);

            switch (c) {
            case 'o':
                base = 8;
                goto ultoa;
            case 'p':
                flags |= FL_ALT;
                /* no break */
            case 'x':
                if (flags & FL_ALT)
                    flags |= FL_ALTHEX;
                base = 16;
                goto ultoa;
            case 'X':
                if (flags & FL_ALT)
                    flags |= (FL_ALTHEX | FL_ALTUPP);
                base = 16 | XTOA_UPPER;
ultoa:
                if (flags & FL_LONGLONG) {
                    c = ulltoa_invert (va_arg(ap, unsigned long long),
                                       (char *)buf, base)  -  (char *)buf;
                } else {
                    c = ultoa_invert ((flags & FL_LONG)
                                      ? va_arg(ap, unsigned long)
                                      : va_arg(ap, unsigned int),
                                      (char *)buf, base)  -  (char *)buf;
                }
                flags &= ~FL_NEGATIVE;
                break;

            default:
                return 0;
            }
        }

        /*
         * Format integers.
         */
        {
            unsigned char len;

            len = c;
            if (flags & FL_PREC) {
                flags &= ~FL_ZFILL;
                if (len < prec) {
                    len = prec;
                    if ((flags & FL_ALT) && !(flags & FL_ALTHEX)) {
                        flags &= ~FL_ALT;
                    }
                }
            }
            if (flags & FL_ALT) {
                if (buf[c-1] == '0') {
                    flags &= ~(FL_ALT | FL_ALTHEX | FL_ALTUPP);
                } else {
                    len += 1;
                    if (flags & FL_ALTHEX) {
                        len += 1;
                    }
                }
            } else if (flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) {
                len += 1;
            }

            if (!(flags & FL_LPAD)) {
                if (flags & FL_ZFILL) {
                    prec = c;
                    if (len < width) {
                        prec += width - len;
                        len = width;
                    }
                }
                while (len < width) {
                    early_putchar(' ');
                    len++;
                }
            }

            width =  (len < width) ? width - len : 0;

            if (flags & FL_ALT) {
                early_putchar('0');
                if (flags & FL_ALTHEX) {
                    early_putchar(flags & FL_ALTUPP ? 'X' : 'x');
                }
            } else if (flags & (FL_NEGATIVE | FL_PLUS | FL_SPACE)) {
                unsigned char z = ' ';
                if (flags & FL_PLUS) {
                    z = '+';
                }
                if (flags & FL_NEGATIVE) {
                    z = '-';
                }
                early_putchar(z);
            }

            while (prec > c) {
                early_putchar('0');
                prec--;
            }

            do {
                early_putchar(buf[--c]);
            } while (c);
        }

tail:
        /* Tail is possible.    */
        while (width) {
            early_putchar(' ');
            width--;
        }
    } /* for (;;) */

    return 0;
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

