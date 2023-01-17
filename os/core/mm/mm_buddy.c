#include <chinos/config.h>

#include <k_stdio.h>
#include <k_string.h>
#include <uapi/util.h>

#include "mm_buddy.h"

#ifdef CONFIG_MM_BUDDY

static int buddy_list_index(int len)
{
    return len - 1;
}

static int buddy_list_index_size(int index)
{
    return index + 1;
}

static unsigned int buddy_metasize(unsigned int maxsections)
{
	/*buddy_t中已经带有一个sections_t的空间，因此后面可以少分配一个section_t空间*/
    return sizeof(buddy_t) + sizeof(section_t) * (maxsections - 1);
}

// initial the pages and insert it to freelist
static void add_free_pages(buddy_t* buddy, page_t *freepage, unsigned long base, unsigned int total)
{
    while(total > 0)
    {
    	/*复合页面最多包括64个page*/
        unsigned int len = total > CONFIG_BUDDY_MAX_LEN ? CONFIG_BUDDY_MAX_LEN : total;
        
        if(len > 1)
            page_compound_init(freepage, base, len);
        else
			/*只有一个页面时*/
            page_init(freepage, base);
        
		list_add_tail(&freepage->list, buddy->list + buddy_list_index(len));
        
        total -= len;
        base += len << CONFIG_PAGE_SHIFT;
        freepage += len;
    }
}

static int add_section(buddy_t *buddy, void *block, unsigned int count, unsigned int offset)
{
    unsigned int metasize;
    page_t *pages;
    unsigned int bitmapsize = bitmap_size(count);
    bitmap_t *bitmap;
    unsigned int freebegin;
    unsigned int secindex;
    unsigned int i;

	/*buddy系统段属性的元数据大小4字节对齐*/
    offset = ALIGN(offset, sizeof(unsigned long));
	/*pages结构体的起始地址*/
    pages = (void*)((unsigned long)block + offset);
	/*段属性和页面结构体总共的元数据占用空间大小，每一页都由一个page_t进行管理，同样需要4字节对齐*/
    metasize =  offset + ALIGN(sizeof(page_t) * count, sizeof(unsigned long));
    /*计算位图结构体的起始地址，此处原代码为：pages + metasize，多加了一次offset的偏移
	更新为block + metasize*/
    bitmap = (void*)((unsigned long)block + metasize);
	/*更新加上位图后的元数据大小*/
    metasize += ALIGN(bitmapsize, sizeof(unsigned long));
    
    if(buddy->seccount == buddy->maxseccount || metasize > (count * CONFIG_PAGE_SIZE))
        return -1;

	/*计算去掉元数据占用的页面后，剩余可用页面起始下标*/
    freebegin = ALIGN(metasize, CONFIG_PAGE_SIZE) >> CONFIG_PAGE_SHIFT;
	/*初始化位图为0*/
    bitmap_init(bitmap, count, 0);
	/*将0~freebegin范围内的页面的位图进行赋值，赋值为1代表此页面已经被使用，freebegin前的页面存放的都是元数据*/
    bitmap_sets(bitmap, 0, freebegin);
	/*将剩余的页面添加到buddy系统中，页面结构体同样需要偏移freebegin，起始地址偏移freebegin << CONFIG_PAGE_SHIFT)
	剩余页面：count - freebegin*/
    add_free_pages(buddy, pages + freebegin, (unsigned long)block + (freebegin << CONFIG_PAGE_SHIFT), count - freebegin);

	/*将buddy系统的所有元数据占用的页面进行初始化，并全部设置为被锁定状态，防止被误分配*/
    while(freebegin > 0)
    {
        freebegin--;
        page_init(pages + freebegin, (unsigned long)block + (freebegin << CONFIG_PAGE_SHIFT));
        page_mark_locked(pages + freebegin);
    }
    
    secindex = 0;
	/*找到需要添加的段在buddy系统段属性中的下标*/
    while(secindex < buddy->seccount && buddy->secs[secindex].begin < (unsigned long)block) 
		secindex++;
    
    i = buddy->seccount;
	/*如果i比secindex大，说明需要添加的段在原有的段中起始地址处于中间位置，将起始位置（即begin成员）比需要添加的段
	起始地址（即block）大的依次向后面挪一位*/
    while(i > secindex)
    {
        k_memcpy(buddy->secs + i, buddy->secs + i - 1, sizeof(section_t));
        i--;
    }

	/*初始化新增的段属性*/
    buddy->secs[secindex].begin = (unsigned long)block;
    buddy->secs[secindex].end = (unsigned long)block + (count << CONFIG_PAGE_SHIFT) - 1;
    buddy->secs[secindex].bitmap = bitmap;
    buddy->secs[secindex].size = count;
    buddy->secs[secindex].pages = pages;
	
    /*已有段数量加1*/
    buddy->seccount++;
    return 0;
}

// return the index of the page
/*首先通过基地址：block找到对应的段下标:secindex，然后在段中找到对应页面的下标:ret*/
static int section_find_block(buddy_t *buddy, unsigned long begin, unsigned int *secindex)
{
    int i;
    int ret = -1;
    
    for(i = 0; i < buddy->seccount; i++)
    {
        if(buddy->secs[i].begin <= begin && buddy->secs[i].end >= begin)
        {
            unsigned int offset = begin - (unsigned long) buddy->secs[i].begin;
            
             // get the page index
            offset >>= CONFIG_PAGE_SHIFT;
            ret = offset;
            *secindex = i;
            break;
        }
    }
    
    return ret;
}
/*begin:需要释放的页面基地址，secindex：页面基地址所在的段index，ret：返回页面基地址在对应的
段index中的页面下标*/
static int section_find_page(buddy_t *buddy, page_t* begin, unsigned int *secindex)
{
    int i;
    int ret = -1;
    
    for(i = 0; i < buddy->seccount; i++)
    {
    	/*根据buddy结构体中的pages和size判断给定页面基地址是否在当前段中*/
        if(begin > buddy->secs[i].pages && begin - buddy->secs[i].pages < buddy->secs[i].size)
        {
            *secindex = i;
            ret = begin - buddy->secs[i].pages;
        }
    }
    
    return ret;
}

///
/// @base must be aligned to page size
///
/*base：需要进行对齐的基地址，src：一次能分配的最大页面数，target：需要对齐的页面数量，单位为页面*/
static inline int base_aligned_offset(unsigned long base, unsigned int src, unsigned int target)
{
    unsigned long alignbase;

    assert((base & bitmask(CONFIG_PAGE_SHIFT)) == 0);
    
    base >>= CONFIG_PAGE_SHIFT;
    alignbase = base + target - 1;
    alignbase /= target;
    alignbase *= target;

	/*如果分配src个页面还是不能满足对齐，直接返回*/
    if(base + src < alignbase + target)
        return -1;
    else
		/*返回对齐需要额外分配的页面数*/
        return alignbase - base;
}

/*buddy:buddy系统管理结构体，pages:需要分配的页面数, pagebegin:二级指针，用于保存分配到的
页面的地址，needalign：是否需要对齐*/
static void* alloc_internal(buddy_t *buddy, int pages, page_t **pagebegin, int needalign)
{    
    int i;
    page_t *p;
    void* ret = 0;
    
    *pagebegin = 0;
    if(pages > CONFIG_BUDDY_MAX_LEN || pages <= 0)
        return ret;
    
    for(i = buddy_list_index(pages); i < CONFIG_BUDDY_MAX_LEN; i++)
    {
        if(!list_empty(&buddy->list[i]))
        {
        	/*分配完成后还剩下的页面数*/
            int left = buddy_list_index_size(i) - pages;
            unsigned long base;
            unsigned int secindex;
            unsigned int pageindex;
            int offset = -1;
            /*通过buddy->list[i].next的地址找到所在page_t的首地址*/
            p = container_of(buddy->list[i].next, page_t, list);
			/*如果p->list的地址等于buddy->list[i]，说明当前buddy系统的list[i]已经遍历完*/
            while(&p->list != &buddy->list[i])
            {
            	/*复合页面链表中第一个页面一定是root页面，base成员存放的是复合页面的基地址*/
                base = (unsigned long) page_get_block(p);
				/*是否需要对齐，如果需要对齐，计算对齐需要的偏移*/
                offset = (needalign == 0) ? 0 : base_aligned_offset(base, buddy_list_index_size(i), pages);
                /*说明当前buddy系统链表能够满足需求*/
                if(offset >= 0)
                {
                	/*从buddy系统链表中删除，等待分配完成后再 插入对应的buddy系统链表*/
					list_del_init(&p->list);
					
                    pageindex = section_find_block(buddy, base, &secindex);
                    break;
                }
                /*走到此处说明，当前页面链表不满足需求，找到下一个页面链表*/
                p = container_of(p->list.next, page_t, list);
            }
            /*走到此处说明当前的buddy系统链表不满足需求，遍历下一个链表*/
            if(offset == -1)
                continue; // search the next buddy list
            
            if(offset > 0)
            {
                // it must be a compound page and still have some pages left
                page_compound_shrink(p, offset);
				/*由于需要满足偏移，将用于填充偏移的页面链表添加到对应的buddy系统链表中*/
				list_add_tail(&p->list, buddy->list + buddy_list_index(offset));

				/*进行对齐操作*/
                p += offset;
                base += offset << CONFIG_PAGE_SHIFT;
                left -= offset;
				/*初始化进行对齐操作后的复合页面链表*/
                page_compound_init(p, base, buddy_list_index_size(i) - offset);
            }

			/*如果剩余页面都还满足分配需求，且不需要进行对齐*/
            if(left > pages && needalign == 0)
            {
            	/*从页面链表尾部开始分配，尽量保留长的页面链表*/
                *pagebegin = p + left;
                
                page_compound_shrink(p, left);
				/*计算分配出来的空间的基地址*/
                ret = (void*) (base + (left << CONFIG_PAGE_SHIFT));
				/*将分配后剩余的页面链表加入到对应的buddy系统链表中*/
                list_add_tail(&p->list, buddy->list + buddy_list_index(left));
                
                if(pages > 1)
                    page_compound_init(*pagebegin, (unsigned long)ret, pages);
                else
                    page_init(*pagebegin, (unsigned long)ret);
                
                bitmap_sets(buddy->secs[secindex].bitmap, pageindex + left, pages);
            }
			/*当前情况为0 < left < pages,不关心是否对齐，或者是left > pages & 需要对齐*/
            else if(left > 0)
            {
            	/*从当前位置开始分配，不从尾部开始分配页面*/
                *pagebegin = p;
                
                page_compound_shrink(p, pages);
                ret = (void*) base;

				/*将分配后剩余的页面链表加入到对应的buddy系统链表中*/
                add_free_pages(buddy, p + pages, base + (pages << CONFIG_PAGE_SHIFT), left);
                bitmap_sets(buddy->secs[secindex].bitmap, pageindex, pages);
            }
			/*当前情况为left <= 0, 此时最坏的情况只能是等于0，小于0的情况不会出现，因为我们是
			直接从buddy系统链表中页面链表中页面数等于pages的链表中依次向上遍历的，因此当前情况
			下不会产生剩下的页面链表*/
            else
            {
                *pagebegin = p;
                ret = (void *) base;
                bitmap_sets(buddy->secs[secindex].bitmap, pageindex, pages);
            }
            
            break; // done
        }
    }
    /*返回分配的页面基地址*/
    return ret;
}

/*通过block基地址从伙伴系统中找到对应的root page页面*/
page_t* buddy_find_page(buddy_t *buddy, void *block)
{
    unsigned int secindex = 0;
    int ret = 0;
    page_t *page = 0;
	
    ret = section_find_block(buddy, (unsigned long)block, &secindex);
    
    if(ret >= 0)
    {
    	/*找到对应的page结构体*/
        page = buddy->secs[secindex].pages + ret;
		/*找到对应的root页面*/
        page = page_get_root(page);
    }
    
    return page;
}

void* buddy_alloc(buddy_t *buddy, int pages)
{
    page_t *page;
    void* ret;
    
    ret = alloc_internal(buddy, pages, &page, 0);
    return ret;
}

/*分配需要按照pages个页面size对齐的pages个页面的空间*/
void* buddy_alloc_aligned(buddy_t *buddy, int pages)
{
    page_t *page;
    void* ret;
    
    ret = alloc_internal(buddy, pages, &page, 1);
    return ret;
}

void buddy_free(buddy_t *buddy, void* block)
{
    unsigned int secindex;
    unsigned int size;
    int pageindex = section_find_block(buddy, (unsigned long)block, &secindex);
    page_t *root;
    
    if(pageindex >= 0)
    {
        root = page_get_root(buddy->secs[secindex].pages + pageindex);
        size = page_get_size(root);
        
		list_add_tail(&root->list, buddy->list + buddy_list_index(size));
        bitmap_clean(buddy->secs[secindex].bitmap, pageindex, size);
        // todo: merge small blocks
    }
    // todo: failed to free the block
}

page_t *buddy_alloc_pages(buddy_t *buddy, int pages)
{
    page_t *page;
    alloc_internal(buddy, pages, &page, 0);
    return page;
}

void buddy_free_pages(buddy_t *buddy, page_t* begin, int pages)
{
    unsigned int secindex;
    int pageindex = section_find_page(buddy, begin, &secindex);
    unsigned long base;
    
    assert(pages < CONFIG_BUDDY_MAX_LEN);
    
    if(pageindex >= 0)
    {
    	/*得到要释放页面的起始基地址*/
        base = buddy->secs[secindex].begin + (pageindex << CONFIG_PAGE_SHIFT);
		/*释放后，这些页面又可以满足分配，初始化为复合页面*/
        page_compound_init(begin, base, pages);
       	/*清除页面位图，标记对应页面为未使用状态*/
        bitmap_clean(buddy->secs[secindex].bitmap, pageindex, pages);
		/*将新生成的复合页面加入到对应的buddy系统链表中*/
        list_add_tail(&begin->list, buddy->list + buddy_list_index(pages));
        // todo: merge small blocks
    }
	
    // todo: failed to free the block
}

int buddy_add_section(buddy_t *buddy, void *block, unsigned int pages)
{
    return add_section(buddy, block, pages, 0);
}

buddy_t* buddy_init(void *block, unsigned int pages, unsigned int maxsections)
{
	/*计算buddy系统元数据需要占用的空间*/
    unsigned int metasize = buddy_metasize(maxsections);
    buddy_t *buddy = 0;
    int i;
    
    if(metasize <= (pages << CONFIG_PAGE_SHIFT) && maxsections > 0)
    {
        buddy = block;
		/*buddy系统共有2的6次方个链表，即64种可分配的链表*/
        for(i = 0; i < CONFIG_BUDDY_MAX_LEN; i++) {
            INIT_LIST_HEAD(buddy->list + i);
        }
        buddy->maxseccount = maxsections;
        buddy->seccount = 0;
        
        if(add_section(buddy, block, pages, metasize) != 0)
            buddy = 0;
    }
    
    return buddy;
}

#endif
