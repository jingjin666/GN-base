#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <syscall.h>
#include <sys/mman.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define x_brk(addr)         ((uintptr_t)__syscall(SYS_brk, addr));
#define x_mmap              __mmap
#define MMAP_THRESHOLD      (128*1024)
  
#define MIN_EXTEND          (2 * SIZEOF_MM_ALLOCNODE)
  
#define MM_MIN_SHIFT	    (5)		/* 32 bytes */
#define MM_MAX_SHIFT	    (22)	/*  4 Mb */
    
#define MM_MIN_CHUNK        (1 << MM_MIN_SHIFT)
#define MM_MAX_CHUNK        (1 << MM_MAX_SHIFT)
#define MM_NNODES           (MM_MAX_SHIFT - MM_MIN_SHIFT + 1)
    
#define MM_GRAN_MASK        (MM_MIN_CHUNK - 1)
#define MM_ALIGN_UP(a)      (((a) + MM_GRAN_MASK) & ~MM_GRAN_MASK)
#define MM_ALIGN_DOWN(a)    ((a) & ~MM_GRAN_MASK)

#define MM_ALLOC_BIT            (0x80000000)

#define SIZEOF_MM_ALLOCNODE     (16)
#define MM_PTR_SIZE             sizeof(struct mm_freenode *)
#define SIZEOF_MM_FREENODE      (SIZEOF_MM_ALLOCNODE + 2*MM_PTR_SIZE)

#define CHECK_ALLOCNODE_SIZE \
  assert(sizeof(struct mm_allocnode) == SIZEOF_MM_ALLOCNODE)

#define CHECK_FREENODE_SIZE \
  assert(sizeof(struct mm_freenode) == SIZEOF_MM_FREENODE)

/*****************************************************************************
 * Private Types
 *****************************************************************************/

typedef struct mm_allocnode
{
    size_t size;            /* Size of this chunk */
    size_t preceding;       /* Size of the preceding chunk */
} mm_allocnode_t;

typedef struct mm_freenode
{
    size_t size;                  /* Size of this chunk */
    size_t preceding;             /* Size of the preceding chunk */
    struct mm_freenode* next;       /* Supports a doubly linked list */
    struct mm_freenode* prev;
} mm_freenode_t;

typedef struct mm_heap
{
    bool is_init;
    size_t mm_heapsize;

    struct mm_allocnode* mm_heapstart;
    struct mm_allocnode* mm_heapend;

    struct mm_freenode mm_nodelist[MM_NNODES];
} mm_heap_t;


/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int   mm_size2ndx(size_t size);
static void  slob_free(void *mem);
static void  mm_addfreechunk(struct mm_heap *heap, struct mm_freenode *node);
static void  mm_addregion(struct mm_heap *heap, void *heapstart, size_t heapsize);
static void  slob_init(void);
static void  slob_extend(struct mm_heap *heap, void *mem, size_t size);
static void *slob_malloc(size_t size);
static void  slob_free(void *mem);

/*****************************************************************************
 * Private Data
 *****************************************************************************/

static struct mm_heap slob;
static uintptr_t c_brk;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mm_size2ndx(size_t size)
{
    int ndx = 0;

    if (size >= MM_MAX_CHUNK)
    {
        return MM_NNODES - 1;
    }

    size >>= MM_MIN_SHIFT;
    while (size > 1)
    {
        ndx++;
        size >>= 1;
    }

    return ndx;
}

static void mm_addfreechunk(struct mm_heap *heap, struct mm_freenode *node)
{
	struct mm_freenode *next;
	struct mm_freenode *prev;
	int ndx;

	assert(node->size >= SIZEOF_MM_FREENODE);
	assert((node->preceding & MM_ALLOC_BIT) == 0);

	/* Convert the size to a nodelist index */

	ndx = mm_size2ndx(node->size);

	/* Now put the new node into the next */

	for (prev = &heap->mm_nodelist[ndx],
		next = heap->mm_nodelist[ndx].next;
		next && next->size && next->size < node->size;
		prev = next, next = next->next);

	/* Does it go in mid next or at the end? */

	prev->next = node;
	node->prev = prev;
	node->next = next;

	if (next)
	{
		/* The new node goes between prev and next */

		next->prev = node;
	}
}

static void mm_addregion(struct mm_heap *heap, void *heapstart, size_t heapsize)
{
	struct mm_freenode *node;
	uintptr_t heapbase;
	uintptr_t heapend;

	heapbase = MM_ALIGN_UP((uintptr_t)heapstart);
	heapend = MM_ALIGN_DOWN((uintptr_t)heapstart + (uintptr_t)heapsize);
	//printf("heapbase = %p heapend = %p\n", heapbase, heapend);

	heapsize = heapend - heapbase;

	/* Add the size of this region to the total size of the heap */

	heap->mm_heapsize += heapsize;

	/* Create two "allocated" guard nodes at the beginning and end of
	 * the heap.  These only serve to keep us from allocating outside
	 * of the heap.
	 *
	 * And create one free node between the guard nodes that contains
	 * all available memory.
	 */

	heap->mm_heapstart = (struct mm_allocnode *)heapbase;
	heap->mm_heapstart->size = SIZEOF_MM_ALLOCNODE;
	heap->mm_heapstart->preceding = MM_ALLOC_BIT;

	node = (struct mm_freenode *)(heapbase + SIZEOF_MM_ALLOCNODE);
	node->size = heapsize - 2 * SIZEOF_MM_ALLOCNODE;
	node->preceding = SIZEOF_MM_ALLOCNODE;

	heap->mm_heapend = (struct mm_allocnode *)(heapend - SIZEOF_MM_ALLOCNODE);
	heap->mm_heapend->size = SIZEOF_MM_ALLOCNODE;
	heap->mm_heapend->preceding = node->size | MM_ALLOC_BIT;

	/* Add the single, large free node to the nodelist */

	mm_addfreechunk(heap, node);
}

static void slob_init(void)
{
	struct mm_heap *heap = &slob;
	int i;

    if (heap->is_init) {
        return ;
    }

    uintptr_t heapstart = x_brk(0);
    size_t heapsize = MMAP_THRESHOLD;
    c_brk = x_brk(heapstart + heapsize);

    assert(heapstart && heapsize > 0);
	//printf("Heap: start=%p size=0x%x\n", heapstart, heapsize);

    CHECK_ALLOCNODE_SIZE;
    CHECK_FREENODE_SIZE;

	assert(MM_MIN_CHUNK >= SIZEOF_MM_FREENODE);
	assert(MM_MIN_CHUNK >= SIZEOF_MM_ALLOCNODE);

	/* Set up global variables */

	memset(heap, 0, sizeof(struct mm_heap));

	/* Initialize the node array */

	for (i = 1; i < MM_NNODES; i++)
	{
		heap->mm_nodelist[i - 1].next = &heap->mm_nodelist[i];
		heap->mm_nodelist[i].prev = &heap->mm_nodelist[i - 1];
	}

	/* Add the initial region of memory to the heap */

	mm_addregion(heap, (void *)heapstart, heapsize);

    heap->is_init = true;
}

static void slob_extend(struct mm_heap *heap, void *mem, size_t size)
{
    struct mm_allocnode *oldnode;
    struct mm_allocnode *newnode;
    uintptr_t blockstart;
    uintptr_t blockend;

    /* Make sure that we were passed valid parameters */

    assert(heap && mem);
    assert(size >= MIN_EXTEND);

    /* Make sure that the memory region are properly aligned */

    blockstart = (uintptr_t)mem;
    blockend   = blockstart + size;

    assert(MM_ALIGN_UP(blockstart) == blockstart);
    assert(MM_ALIGN_DOWN(blockend) == blockend);


    /* Get the terminal node in the old heap.  The block to extend must
    * immediately follow this node.
    */

    oldnode = heap->mm_heapend;
    assert((uintptr_t)oldnode + SIZEOF_MM_ALLOCNODE == (uintptr_t)mem);

    /* The size of the old node now extends to the new terminal node.
    * This is the old size (SIZEOF_MM_ALLOCNODE) plus the size of
    * the block (size) minus the size of the new terminal node
    * (SIZEOF_MM_ALLOCNODE) or simply:
    */

    oldnode->size = size;

    /* The old node should already be marked as allocated */

    assert((oldnode->preceding & MM_ALLOC_BIT) != 0);

    /* Get and initialize the new terminal node in the heap */

    newnode            = (struct mm_allocnode *)(blockend - SIZEOF_MM_ALLOCNODE);
    newnode->size      = SIZEOF_MM_ALLOCNODE;
    newnode->preceding = oldnode->size | MM_ALLOC_BIT;

    heap->mm_heapend = newnode;

    /* Finally "free" the new block of memory where the old terminal node was
    * located.
    */

    slob_free((void *)mem);
}

static void *slob_malloc(size_t size)
{
    struct mm_heap *heap = &slob;
	struct mm_freenode *node;
	size_t alignsize;
	void *ret = NULL;
	int ndx;

    if (size <= 0)
        return NULL;

    if (!heap->is_init) {
        slob_init();
    }

    if (size >= MMAP_THRESHOLD) {
        //return x_mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    }

	alignsize = MM_ALIGN_UP(size + SIZEOF_MM_ALLOCNODE);
	//printf("alignsize = %zu\n", alignsize);
	if (alignsize < size)
	{
		/* There must have been an integer overflow */

		return NULL;
	}

	assert(alignsize >= MM_MIN_CHUNK);
	assert(alignsize >= SIZEOF_MM_FREENODE);

	if (alignsize >= MM_MAX_CHUNK)
	{
		ndx = MM_NNODES - 1;
	}
	else
	{
		/* Convert the request size into a nodelist index */

		ndx = mm_size2ndx(alignsize);
	}

	/* Search for a large enough chunk in the list of nodes. This list is
	 * ordered by size, but will have occasional zero sized nodes as we visit
	 * other mm_nodelist[] entries.
	 */

	for (node = heap->mm_nodelist[ndx].next;
		node && node->size < alignsize;
		node = node->next, ndx++)
	{
		assert(node->prev->next == node);
	}

	/* If we found a node with non-zero size, then this is one to use. Since
	 * the list is ordered, we know that it must be the best fitting chunk
	 * available.
	 */
	if (node)
	{
		//printf("node = %p, size = %p, preceding = %p\n", node, node->size, node->preceding);
		struct mm_freenode* remainder;
		struct mm_freenode* next;
		uint32_t remaining;

		/* Remove the node.  There must be a predecessor, but there may not be
		 * a successor node.
		 */

		assert(node->prev);
		node->prev->next = node->next;
		if (node->next)
		{
			node->next->prev = node->prev;
		}

		/* Check if we have to split the free node into one of the allocated
		 * size and another smaller freenode.  In some cases, the remaining
		 * bytes can be smaller (they may be SIZEOF_MM_ALLOCNODE).  In that
		 * case, we will just carry the few wasted bytes at the end of the
		 * allocation.
		 */

		remaining = node->size - alignsize;
		if (remaining >= SIZEOF_MM_FREENODE)
		{
			/* Get a pointer to the next node in physical memory */

			next = (struct mm_freenode*)(((char*)node) + node->size);

			/* Create the remainder node */

			remainder = (struct mm_freenode*)(((char*)node) + alignsize);

			remainder->size = remaining;
			remainder->preceding = alignsize;

			/* Adjust the size of the node under consideration */

			node->size = alignsize;

			/* Adjust the 'preceding' size of the (old) next node, preserving
			 * the allocated flag.
			 */

			next->preceding = remaining | (next->preceding & MM_ALLOC_BIT);

			/* Add the remainder back into the nodelist */

			mm_addfreechunk(heap, remainder);
		}

		/* Handle the case of an exact size match */

		node->preceding |= MM_ALLOC_BIT;
		ret = (void*)((char*)node + SIZEOF_MM_ALLOCNODE);
	}
    else
    {
        uintptr_t old_brk = c_brk;
        c_brk = x_brk(old_brk + MMAP_THRESHOLD);
        slob_extend(heap, (void *)old_brk, MMAP_THRESHOLD);
        return slob_malloc(size);
    }

	return ret;
}

static void slob_free(void *mem)
{
    struct mm_heap *heap = &slob;
	struct mm_freenode *node;
	struct mm_freenode *prev;
	struct mm_freenode *next;

    assert(mem);
	//printf("Freeing %p\n", mem);

	/* Protect against attempts to free a NULL reference */

	if (!mem)
	{
		return;
	}

	/* Map the memory chunk into a free node */

	node = (struct mm_freenode *)((char*)mem - SIZEOF_MM_ALLOCNODE);

	/* Sanity check against double-frees */

	assert(node->preceding & MM_ALLOC_BIT);

	node->preceding &= ~MM_ALLOC_BIT;

	/* Check if the following node is free and, if so, merge it */

	next = (struct mm_freenode *)((char *)node + node->size);
	assert((next->preceding & ~MM_ALLOC_BIT) == node->size);
	if ((next->preceding & MM_ALLOC_BIT) == 0)
	{
		struct mm_allocnode *andbeyond;

		/* Get the node following the next node (which will
		 * become the new next node). We know that we can never
		 * index past the tail chunk because it is always allocated.
		 */

		andbeyond = (struct mm_allocnode *)((char *)next + next->size);

		/* Remove the next node.  There must be a predecessor,
		 * but there may not be a successor node.
		 */

		assert(next->prev);
		next->prev->next = next->next;
		if (next->next)
		{
			next->next->prev = next->prev;
		}

		/* Then merge the two chunks */

		node->size += next->size;
		andbeyond->preceding = node->size | (andbeyond->preceding & MM_ALLOC_BIT);
		next = (struct mm_freenode *)andbeyond;
	}

	/* Check if the preceding node is also free and, if so, merge
	 * it with this node
	 */

	prev = (struct mm_freenode *)((char *)node - node->preceding);
	assert((node->preceding & ~MM_ALLOC_BIT) == prev->size);
	if ((prev->preceding & MM_ALLOC_BIT) == 0)
	{
		/* Remove the node.  There must be a predecessor, but there may
		 * not be a successor node.
		 */

		assert(prev->prev);
		prev->prev->next = prev->next;
		if (prev->next)
		{
			prev->next->prev = prev->prev;
		}

		/* Then merge the two chunks */

		prev->size += node->size;
		next->preceding = prev->size | (next->preceding & MM_ALLOC_BIT);
		node = prev;
	}

	/* Add the merged node to the nodelist */

	mm_addfreechunk(heap, node);
}

weak_alias(slob_malloc, malloc);
weak_alias(slob_free, __libc_free);
