#include <chinos/config.h>

#include <k_limits.h>
#include <uapi/util.h>

#include "mm_bitmap.h"

#ifdef CONFIG_MM_BUDDY

static inline unsigned long clz(unsigned long val)
{
    unsigned long ret = 0;

    int bit = sizeof(unsigned long) * 8 - 1;
    while (bit >= 0)
    {
        if (((val >> bit) & 0x1) == 0)
            ret++;
        else
            break;
        bit--;
    }

    return ret;
}

unsigned int bitmap_size(unsigned int objs)
{
    unsigned int size;

    /*将objs 进行32字节对齐，一个字节有8位，除以8代表位图需要使用多少字节，保证size是四字节对齐*/
    size = ALIGN(objs, sizeof(unsigned long)*8) / 8;
	/*位图结构体占用的空间大小，位图结构体中bits成员已经提供四个字节可供位图使用，即总占用大小需要减去
	bits成员大小*/
    size += sizeof(bitmap_t) - sizeof(unsigned long);

    return size;
}

void bitmap_sets(bitmap_t *bitmap, unsigned int begin, unsigned int objs)
{
	/*一个页面占用一个bit，此处设置的位图起始处可能会占用某一四字节位图中的某些bit*/
    unsigned int remain = begin % WORD_BITS;
	/*一个页面占用一个bit，计算此处设置的位图起始处已占用的起始位图地址*/
    unsigned long *bit = bitmap->bits + (begin / WORD_BITS);

    if(remain > 0)
    {
    	/*该四字节位图剩余的bit*/
        unsigned int count = WORD_BITS - remain;
        /*计算此次需要设置的页面数（即位图bit数）是否能够占满剩下的bit*/
        count = objs > count ? count : objs;
        objs -= count;
        /*将此次需要设置的页面数bit置1*/
        *bit |= bitmask(count) << remain;
		/*不论此时是否能够占满当前四字节位图，就移到下一个四字节位图*/
        bit++;
    }
    /*计算objs实际需要占用多少四字节位图，将占用的bit全部置1*/
    while(objs >= WORD_BITS)
    {
        *bit = ~0;
        bit++;
        objs -= WORD_BITS;
    }
    /*剩余的页面数不够占用完整的32bit，将占用的bit置1*/
    if(objs > 0)
        *bit |= bitmask(objs);
}

void bitmap_clean(bitmap_t *bitmap, unsigned int begin, unsigned int objs)
{
    unsigned int remain = begin % WORD_BITS;
    unsigned long *bit = bitmap->bits + (begin / WORD_BITS);
    
    if(remain > 0)
    {
        unsigned int count = WORD_BITS - remain;
        
        count = objs > count ? count : objs;
        objs -= count;
        
        *bit &= ~(bitmask(count) << remain);
        bit++;
    }
    
    while(objs >= WORD_BITS)
    {
        *bit = 0;
        bit++;
        objs -= WORD_BITS;
    }
    
    if(objs > 0)
        *bit &= ~bitmask(objs);
}

void bitmap_init(bitmap_t *bitmap, unsigned int objs, unsigned int val)
{
    unsigned long value = val > 0 ? (~0) : 0;
    bitmap_t* bm = (void*)bitmap;
    unsigned long *bit = bm->bits;

	/*计算需要占用的4字节空间（一个unsigned int占用4字节）数量，减去sizeof(unsigned long)是为了去掉
	位图结构体中objs占用的空间*/
    unsigned int bitcount = (bitmap_size(objs) - sizeof(unsigned long))/sizeof(unsigned long);
    
    bm->objs = objs;
    while(bitcount > 0)
    {
        *bit++ = value;
        bitcount--;
    }
}

#endif
