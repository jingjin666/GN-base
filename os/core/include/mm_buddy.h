#ifndef __MM_BUDDY_H
#define __MM_BUDDY_H

#include <chinos/config.h>

#include "mm_page.h"
#include "mm_bitmap.h"

#ifdef CONFIG_MM_BUDDY

#define CONFIG_BUDDY_MAX_LEN (1 << 11)

#define CONFIG_PAGE_SHIFT (12)

#ifndef CONFIG_PAGE_SIZE
#define CONFIG_PAGE_SIZE (1 << CONFIG_PAGE_SHIFT)
#endif

typedef struct section
{
    unsigned long begin;    // where the block begin
    unsigned long end;      // where the block end

    bitmap_t *bitmap;       // for avaliable pages
    page_t   *pages;        // where the pages start for the block

    unsigned long size;     // the size of this section in page
} section_t;

typedef struct buddy
{
    list_t list[CONFIG_BUDDY_MAX_LEN];

    uint16_t maxseccount;   // max section count
    uint16_t seccount;      // section count
    section_t secs[1];      // C99 only
} buddy_t;

void   *buddy_alloc(buddy_t *buddy, int pages);
void   *buddy_alloc_aligned(buddy_t *buddy, int pages);
void    buddy_free(buddy_t *buddy, void* pages);

page_t *buddy_alloc_pages(buddy_t *buddy, int pages);
page_t* buddy_find_page(buddy_t *buddy, void *block);
void    buddy_free_pages(buddy_t *buddy, page_t* begin, int pages);
int     buddy_add_section(buddy_t *buddy, void *block, unsigned int pages);

buddy_t *buddy_init(void *block, unsigned int pages, unsigned int seccount);

#endif

#endif
