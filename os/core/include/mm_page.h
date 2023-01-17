#ifndef __MM_PAGE_H
#define __MM_PAGE_H

#include <chinos/config.h>

#include <k_assert.h>
#include <k_limits.h>
#include <k_stdint.h>
#include <uapi/list.h>

#ifdef CONFIG_MM_BUDDY

enum __page_flags
{
    PG_LOCKED = 0x1,
	/*复合页面*/
    PG_COMPOUND = 0x10,
    /*复合页面尾部页面*/
    PG_TAIL = 0x20,
    /*被slab子系统申请的页面*/
    PG_SLUB = 0x40,
};

typedef struct __page
{
    unsigned short flags;

    union{
		/*代表当前页面能够满足所属slab分配器的分配次数*/
        unsigned short slab_objs_left;
    };

    union{
		/*页面基地址*/
        unsigned long base;
		/*记录当前页面在所属slab分配器中目前可以分配空间的基地址
		而slab结构体中的freelist主要是代表对当前slab可分配空间基地址
		如果slab->freelist为0，只能代表slab在freelist所在页面无法进行分配，
		此时就要用到slab->partial中的slab_freelist*/
        void *slab_freelist;
    };

    union{
        list_t list;
        struct{
			/*记录当前页面所属slab分配器中的下一个可供分配的部分使用页面*/
            struct __page* slab_partial_next;
			/*当前页面所属slab分配器*/
            void* slab;
        };
    };
} page_t;

static inline void page_init(page_t *page, unsigned long base)
{
    page->flags = 0;
    page->base = base;
}

static inline int page_is_free(page_t *page)
{
    return (page->flags & PG_LOCKED) == 0;
}

static inline void page_mark_locked(page_t *page)
{
    page->flags |= PG_LOCKED;
}

static inline int page_is_compound(page_t *page)
{
    return (page->flags & PG_COMPOUND) > 0;
}

static inline page_t* page_get_root(page_t *page)
{
	/*如果是复合页面，且带有PG_TAIL标志，那么说明前一个页面就是root页面*/
    if(page_is_compound(page) && (page->flags & PG_TAIL))
        return (page_t *)page->list.prev;
    else
		/*此处有两种情况，一种是不是复合页面，说明就是独立的单个页面，直接返回当前page即可
		第二种是是复合页面，但是不带有PG_TAIL标志，说明此页面就是root页面，直接返回*/
        return page;
}

static inline void* page_get_block(page_t *root)
{
    return (void*)root->base;
}

/*复合页面中第二个页面的next成员存放的是当前复合页面的页面数*/
static inline unsigned long page_get_size(page_t *page)
{
    page_t *second = page;
    
    if(page_is_compound(page))
    {
        second = page_get_root(page);
        second++;
        
        return (unsigned long)second->list.next;
    }
    else
        return 1;
}

/*初始化复合页面，复合页面最大是64个page，最小2个page*/
static inline void page_compound_init(page_t *root, unsigned long base, unsigned long size/*in page*/)
{
    page_t *child = root + 1;
    
    //assert(size >= 2);
   
    root->flags = PG_COMPOUND;
    root->base = base;
    
    child->flags = PG_COMPOUND | PG_TAIL;
    child->list.prev = (void*)root;
	/*next成员用于存放复合页面实际占用的页面数？*/
    child->list.next = (void*)size;
    /*root和child各占用一个页面，因此减去2*/
    size -= 2;
    while(size > 0)
    {
        child++;
		/*保证当前复合链表中每个页面都带有PG_TAIL，第一个页面为root页面，第二个页面为
		存放当前复合页面实际占用的页面数，第二个页面到最后一个页面都指向当前复合页面的root页面*/
        child->flags = PG_COMPOUND | PG_TAIL;
        child->list.prev = (void*)root;
        size--;
    }
}

/*返回需要收缩的页面数*/
static inline int page_compound_shrink(page_t *root, unsigned long newsize)
{
    page_t *second = root + 1;
    unsigned long size = page_get_size(root);
    
    assert(size > newsize && newsize > 0);

	/*如果只需要一个页面，去掉root页面的复合标志*/
    if(newsize == 1)
        root->flags &= ~PG_COMPOUND;
    else
		/*否则更新复合页面链表的页面数*/
        second->list.next = (void*) newsize;
    
    return size - newsize;
}

#endif

#endif
