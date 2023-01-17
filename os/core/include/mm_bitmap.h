#ifndef __MM_BITMAP_H
#define __MM_BITMAP_H

#include <chinos/config.h>

#include <k_limits.h>
#include <k_stdint.h>
#include <uapi/util.h>

#ifdef CONFIG_MM_BUDDY

typedef struct __bitmap
{
	/*位图所在段的页面总数*/
    unsigned long objs;
    unsigned long bits[1];
} bitmap_t;

static inline void bitmap_set_bit(bitmap_t *bitmap, unsigned int index)
{
    unsigned int wi = index >> WORD_SHIFT;
    unsigned int bi = index & bitmask(WORD_SHIFT);
    bitmap->bits[wi] |= bit(bi);
}

static inline void bitmap_clean_bit(bitmap_t *bitmap, unsigned int index)
{
    unsigned int wi = index >> WORD_SHIFT;
    unsigned int bi = index & bitmask(WORD_SHIFT);
    bitmap->bits[wi] &= ~bit(bi);
}

static inline unsigned int bitmap_get_objs(bitmap_t *bitmap)
{
    return bitmap->objs;
}

unsigned int bitmap_size(unsigned int objs);
void bitmap_sets(bitmap_t *bitmap, unsigned int begin, unsigned int objs);
void bitmap_clean(bitmap_t *bitmap, unsigned int begin, unsigned int objs);
void bitmap_init(bitmap_t *bitmap, unsigned int objs, unsigned int val);

#endif

#endif
