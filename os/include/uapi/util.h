#ifndef __UAPI_UTIL_H
#define __UAPI_UTIL_H

#include <chinos/config.h>

#include <asm/const.h>

#if defined(CONFIG_ARCH_32)
#define BITS_PER_LONG 32
#elif defined(CONFIG_ARCH_64)
#define BITS_PER_LONG 64
#else
#error "Unkowned ARCH"
#endif

#define KB *1024
#define MB *1024 KB

#define bitmask(x) (UL(1) << (x))
#define lowbitsmask(x) (bitmask(x) - UL(1))

#define align_to(size, align) ((((size) + (align) - 1) / (align))*(align))

#if __GNUC__ > 3
#define offsetof(type, member) __builtin_offsetof(type, member)
#else
#define offsetof(type, member) ((size_t)( (char *)&(((type *)0)->member) - (char *)0 ))
#endif

#define container_of(ptr, type, member) \
  ((type *)((uintptr_t)(ptr) - offsetof(type, member)))

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
    return (value >> begin) & lowbitsmask(width);
}

static inline unsigned long bitfield_clean(unsigned long value, unsigned int begin, unsigned int width)
{
    value &= ~(lowbitsmask(width) << begin);
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
