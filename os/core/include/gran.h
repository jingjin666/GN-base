#ifndef __GRAN_H
#define __GRAN_H

#include <chinos/config.h>

#include <k_stddef.h>
#include <k_stdint.h>

#ifdef CONFIG_MM_GRAN

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Sizes of things */

#define SIZEOF_GAT(n) \
  ((n + 31) >> 5)
#define SIZEOF_MM_GRAN(n) \
  (sizeof(struct mm_gran) + sizeof(uint32_t) * (SIZEOF_GAT(n) - 1))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Form in which the state of the granule allocator is returned */

struct graninfo
{
  uint8_t   log2gran;  /* Log base 2 of the size of one granule */
  uint32_t  ngranules; /* The total number of (aligned) granules in the heap */
  uint32_t  nfree;     /* The number of free granules */
  uint32_t  mxfree;    /* The max continous of free granules */
};

/* This structure represents the state of one granule allocation */

struct mm_gran
{
    uint8_t    log2gran;  /* Log base 2 of the size of one granule */
    uint32_t   ngranules; /* The total number of (aligned) granules in the heap */
    uintptr_t  heapstart; /* The aligned start of the granule heap */
    uint32_t   gat[1];    /* Start of the granule allocation table */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: gran_initialize
 *
 * Description:
 *   Set up one granule allocator instance.  Allocations will be aligned to
 *   the alignment size (log2align; allocations will be in units of the
 *   granule size (log2gran). Larger granules will give better performance
 *   and less overhead but more losses of memory due to quantization waste.
 *   Additional memory waste can occur from alignment; log2align should be
 *   set to 0 unless you are using the granule allocator to manage DMA
 *   or page-aligned memory and your hardware has specific memory alignment
 *   requirements.
 *
 *   General Usage Summary.  This is an example using the GCC section
 *   attribute to position a DMA heap in memory (logic in the linker script
 *   would assign the section .dmaheap to the DMA memory.
 *
 *   uint32_t g_dmaheap[DMAHEAP_SIZE] locate_data(.dmaheap);
 *
 *   The heap is created by calling gran_initialize.  Here the granule size
 *   is set to 64 bytes and the alignment to 16 bytes:
 *
 *     struct mm_gran *gran = gran_initialize(g_dmaheap, DMAHEAP_SIZE, 6, 4);
 *
 *   Then the void * can be used to allocate memory:
 *
 *     uint8_t *dma_memory = (uint8_t *)gran_alloc(handle, 47);
 *
 *   The actual memory allocates will be 64 byte (wasting 17 bytes) and
 *   will be aligned at least to (1 << log2align).
 *
 *   NOTE: The current implementation also restricts the maximum allocation
 *   size to 32 granules.  That restriction could be eliminated with some
 *   additional coding effort.
 *
 * Input Parameters:
 *   heapstart - Start of the granule allocation heap
 *   heapsize  - Size of heap in bytes
 *   log2gran  - Log base 2 of the size of one granule.  0->1 byte,
 *               1->2 bytes, 2->4 bytes, 3->8 bytes, etc.
 *   log2align - Log base 2 of required alignment.  0->1 byte,
 *               1->2 bytes, 2->4 bytes, 3->8 bytes, etc.  Note that
 *               log2gran must be greater than or equal to log2align
 *               so that all contiguous granules in memory will meet
 *               the minimum alignment requirement. A value of zero
 *               would mean that no alignment is required.
 *
 * Returned Value:
 *   On success, a non-NULL handle is returned that may be used with other
 *   granule allocator interfaces.
 *
 ****************************************************************************/

struct mm_gran *gran_initialize(void *heapstart, size_t heapsize, uint8_t log2gran, uint8_t log2align);

/****************************************************************************
 * Name: gran_release
 *
 * Description:
 *   Uninitialize a gram memory allocator and release resources held by the
 *   allocator.
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void gran_release(struct mm_gran *gran);

/****************************************************************************
 * Name: gran_reserve
 *
 * Description:
 *   Reserve memory in the granule heap.  This will reserve the granules
 *   that contain the start and end addresses plus all of the granules
 *   in between.  This should be done early in the initialization sequence
 *   before any other allocations are made.
 *
 *   Reserved memory can never be allocated (it can be freed however which
 *   essentially unreserves the memory).
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *   start  - The address of the beginning of the region to be reserved.
 *   size   - The size of the region to be reserved
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gran_reserve(struct mm_gran *gran, uintptr_t start, size_t size);

/****************************************************************************
 * Name: gran_alloc
 *
 * Description:
 *   Allocate memory from the granule heap.
 *
 *   NOTE: The current implementation also restricts the maximum allocation
 *   size to 32 granules.  That restriction could be eliminated with some
 *   additional coding effort.
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *   size   - The size of the memory region to allocate.
 *
 * Returned Value:
 *   On success, a non-NULL pointer to the allocated memory is returned;
 *   NULL is returned on failure.
 *
 ****************************************************************************/

void *gran_alloc(struct mm_gran *gran, size_t size);

/****************************************************************************
 * Name: gran_free
 *
 * Description:
 *   Return memory to the granule heap.
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *   memory - A pointer to memory previously allocated by gran_alloc.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gran_free(struct mm_gran *gran, void *memory, size_t size);

/****************************************************************************
 * Name: gran_dump
 *
 * Description:
 *   Return information about the granule heap.
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *   info   - Memory location to return the gran allocator info.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is return on
 *   any failure.
 *
 ****************************************************************************/

void gran_dump(struct mm_gran *gran, struct graninfo *info);

/****************************************************************************
 * Name: gran_mark_allocated
 *
 * Description:
 *   Mark a range of granules as allocated.
 *
 * Input Parameters:
 *   priv  - The granule heap state structure.
 *   alloc - The address of the allocation.
 *   ngranules - The number of granules allocated
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gran_mark_allocated(struct mm_gran *priv, uintptr_t alloc, unsigned int ngranules);

#endif
#endif
