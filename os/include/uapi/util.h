#ifndef __UAPI_UTIL_H
#define __UAPI_UTIL_H

#include <chinos/config.h>

#include <k_types.h>
#include <const.h>

#ifdef CONFIG_ARCH_64
#define BITS_PER_LONG       64
#else
#define BITS_PER_LONG       32
#endif
#define BITS_PER_LONG_LONG  64

#ifdef __GNUC__

#define weak __attribute__((__weak__))

#define hidden __attribute__((__visibility__("hidden")))

#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

/* The noreturn attribute informs GCC that the function will not return.
 * C11 adds _Noreturn keyword (see stdnoreturn.h)
 */
#define noreturn_function __attribute__ ((noreturn))
    
/* Code locate */
#define locate_code(n) __attribute__ ((section(n)))
    
/* Data alignment */
#define aligned_data(n) __attribute__ ((aligned(n)))
  
/* Data location */
#define locate_data(n) __attribute__ ((section(n)))
    
/* The packed attribute informs GCC that the structure elements are packed,
 * ignoring other alignment rules.
 */
#define begin_packed_struct
#define end_packed_struct __attribute__ ((packed))
    
/* The unsued code or data */
#define unused_code __attribute__((unused))
#define unused_data __attribute__((unused))

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#else

#define likely
#define unlikely

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef max
#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })
#endif

#ifndef min
#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })
#endif

#ifndef roundup
#define roundup(x, y) (                                \
{                                                      \
	const typeof(y) __y = y;		       \
	(((x) + (__y - 1)) / __y) * __y;	       \
}                                                      \
)
#endif

#define bit(nr) (UL(1) << (nr))
#define bitmask(nr) (bit(nr) - UL(1))

#define ALIGN(size, align) ((((size) + (align) - 1) / (align))*(align))

#define PAGE_ALIGN(size) ALIGN(size, CONFIG_PAGE_SIZE)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Low-level register access */
#define getreg8(a)            (*(volatile uint8_t *)(a))
#define putreg8(v,a)          (*(volatile uint8_t *)(a) = (v))
#define getreg16(a)           (*(volatile uint16_t *)(a))
#define putreg16(v,a)         (*(volatile uint16_t *)(a) = (v))
#define getreg32(a)           (*(volatile uint32_t *)(a))
#define putreg32(v,a)         (*(volatile uint32_t *)(a) = (v))
#define getreg64(a)           (*(volatile uint64_t *)(a))
#define putreg64(v,a)         (*(volatile uint64_t *)(a) = (v))

static inline unsigned long bitfield_get(unsigned long value, unsigned int begin, unsigned int width)
{
    return (value >> begin) & bitmask(width);
}

static inline unsigned long bitfield_clean(unsigned long value, unsigned int begin, unsigned int width)
{
    value &= ~(bitmask(width) << begin);
    return value;
}

static inline unsigned long bitfield_set(unsigned long value, unsigned int begin, unsigned int width, unsigned long bv)
{
    value = bitfield_clean(value, begin, width);
    value |= bv << begin;
    return value;
}

static inline unsigned long BKDRHash(char *str)
{
	unsigned long seed = 131; /* 31 131 1313 13131 131313 etc.. */
	register unsigned long hash = 0;
	unsigned long ch = 0;

	while (*str)
	{
		ch = (unsigned long)*str++;
		hash = hash * seed + ch;
	}

	return hash;
}

#endif
