#include <chinos/config.h>

#include <k_assert.h>
#include <k_stddef.h>
#include <k_stdint.h>

#include "gran.h"

#ifdef CONFIG_MM_GRAN

/****************************************************************************
 * Public Functions
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
 *   The heap is created by calling gran_initialize().  Here the granule size
 *   is set to 64 bytes (2**6) and the alignment to 16 bytes (2**4):
 *
 *     GRAN_HANDLE handle = gran_initialize(g_dmaheap, DMAHEAP_SIZE, 6, 4);
 *
 *   Then the GRAN_HANDLE can be used to allocate memory:
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

struct mm_gran *gran_initialize(void *heapstart, size_t heapsize, uint8_t log2gran, uint8_t log2align)
{
    struct mm_gran    *gran;
    uintptr_t          heapend;
    uintptr_t          alignedstart;
    unsigned long      mask;
    unsigned long      alignedsize;
    unsigned int       ngranules;

    /* 
     * Check parameters if debug is on.  Note the size of a granule is
     * limited to 2**31 bytes and that the size of the granule must be greater
     * than or equal to the alignment size.
     */
    assert(heapstart && heapsize > 0 && log2gran > 0 && log2gran < 32 && log2gran >= log2align);

    /* Get the aligned start of the heap */
    mask         = (1 << log2align) - 1;
    alignedstart = ((uintptr_t)heapstart + mask) & ~mask;

    /* Determine the number of granules */
    mask         = (1 << log2gran) - 1;
    heapend      = (uintptr_t)heapstart + heapsize;
    alignedsize  = (heapend - alignedstart) & ~mask;
    ngranules    = alignedsize >> log2gran;

    /*
     * Allocate the information structure with a granule table of the
     * correct size.
     */
#if 0
    gran = (struct mm_gran *)malloc(SIZEOF_MM_GRAN(ngranules));
#else
    gran = (struct mm_gran *)heapstart;

    unsigned int gran_head = SIZEOF_MM_GRAN(ngranules);
    heapstart = (char *)heapstart + gran_head;
    heapsize = heapsize - gran_head;

    /* Get the aligned start of the heap */
    mask         = (1 << log2align) - 1;
    alignedstart = ((uintptr_t)heapstart + mask) & ~mask;    

    /* Determine the number of granules */
    mask         = (1 << log2gran) - 1;
    heapend      = (uintptr_t)heapstart + heapsize;
    alignedsize  = (heapend - alignedstart) & ~mask;
    ngranules    = alignedsize >> log2gran;
#endif

    if (gran)
    {
        /* Initialize non-zero elements of the granules heap info structure */
        gran->log2gran  = log2gran;
        gran->ngranules = ngranules;
        gran->heapstart = alignedstart;
    }

    return gran;
}

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

void gran_release(struct mm_gran *gran)
{
    assert(gran != NULL);
#if 0
    free(gran);
#endif
}

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

void *gran_alloc(struct mm_gran *gran, size_t size)
{
    unsigned int ngranules;
    size_t       tmpmask;
    uintptr_t    alloc;
    uint32_t     curr;
    uint32_t     next;
    uint32_t     mask;
    int          granidx;
    int          gatidx;
    int          bitidx;
    int          shift;
    int          ret;

    assert(gran != NULL && size <= 32 * (1 << gran->log2gran));

    if (gran != NULL && size > 0)
    {
        /* How many contiguous granules we we need to find? */
        tmpmask   = (1 << gran->log2gran) - 1;
        ngranules = (size + tmpmask) >> gran->log2gran;

        /* Then create mask for that number of granules */
        assert(ngranules <= 32);
        mask = 0xffffffff >> (32 - ngranules);

        /* Now search the granule allocation table for that number of contiguous */
        for (granidx = 0; granidx < gran->ngranules; granidx += 32)
        {
            
            /* Get the GAT index associated with the granule table entry */
            gatidx = granidx >> 5;
            curr = gran->gat[gatidx];

            /* Handle the case where there are no free granules in the entry */
            if (curr == 0xffffffff)
            {
                continue;
            }

            /* Get the next entry from the GAT to support a 64 bit shift */
            if (granidx < gran->ngranules)
            {
                next = gran->gat[gatidx + 1];
            }

            /* Use all ones when are at the last entry in the GAT (meaning nothing can be allocated).*/
            else
            {
                next = 0xffffffff;
            }

            /* 
             *Search through the allocations in the 'curr' GAT entry
             * to see if we can satisfy the allocation starting in that
             * entry.
             *
             * This loop continues until either all of the bits have been
             * examined (bitidx >= 32), or until there are insufficient
             * granules left to satisfy the allocation.
             */
            alloc = gran->heapstart + (granidx << gran->log2gran);

            for (bitidx = 0; bitidx < 32 && (granidx + bitidx + ngranules) <= gran->ngranules; )
            {
                /* 
                 * Break out if there are no further free bits in 'curr'.
                 * All of the zero bits might have gotten shifted out.
                 */
                if (curr == 0xffffffff)
                {
                    break;
                }
                /* 
                 * Check for the first zero bit in the lower or upper 16-bits.
                 * From the test above, we know that at least one of the 32-
                 * bits in 'curr' is zero.
                 */
                else if ((curr & 0x0000ffff) == 0x0000ffff)
                {
                    /* 
                     * Not in the lower 16 bits.  The first free bit must be
                     * in the upper 16 bits.
                     */
                    shift = 16;
                }
                /*
                 * We know that the first free bit is now within the lower 16
                 * bits of 'curr'.  Is it in the upper or lower byte?
                 */
                else if ((curr & 0x0000ff) == 0x000000ff)
                {
                    /*
                     * Not in the lower 8 bits.  The first free bit must be in
                     * the upper 8 bits.
                     */
                    shift = 8;
                }
                /*
                 * We know that the first free bit is now within the lower 4
                 * bits of 'curr'.  Is it in the upper or lower nibble?
                 */
                else if ((curr & 0x00000f) == 0x0000000f)
                {
                    /*
                     * Not in the lower 4 bits.  The first free bit must be in
                     * the upper 4 bits.
                     */
                    shift = 4;
                }
                /*
                 * We know that the first free bit is now within the lower 4
                 * bits of 'curr'.  Is it in the upper or lower pair?
                 */
                else if ((curr & 0x000003) == 0x00000003)
                {
                    /*
                     * Not in the lower 2 bits.  The first free bit must be in
                     * the upper 2 bits.
                     */
                    shift = 2;
                }
                /*
                 * We know that the first free bit is now within the lower 4
                 * bits of 'curr'.  Check if we have the allocation at this
                 * bit position.
                 */
                else if ((curr & mask) == 0)
                {
                    /* Yes.. mark these granules allocated */
                    gran_mark_allocated(gran, alloc, ngranules);

                    /* And return the allocation address */
                    return (void *)alloc;
                }
                /* The free allocation does not start at this position */
                else
                {
                    shift = 1;
                }

                /*
                 * Set up for the next time through the loop.  Perform a 64
                 * bit shift to move to the next gran position and increment
                 * to the next candidate allocation address.
                 */
                alloc  += (shift << gran->log2gran);
                curr    = (curr >> shift) | (next << (32 - shift));
                next  >>= shift;
                bitidx += shift;
            }
        }
    }

    return NULL;
}

void gran_mark_allocated(struct mm_gran *gran, uintptr_t alloc, unsigned int ngranules)
{
    unsigned int granno;
    unsigned int gatidx;
    unsigned int gatbit;
    unsigned int avail;
    uint32_t     gatmask;

    /* Determine the granule number of the allocation */
    granno = (alloc - gran->heapstart) >> gran->log2gran;

    /* Determine the GAT table index associated with the allocation */
    gatidx = granno >> 5;
    gatbit = granno & 31;

    /* Mark bits in the GAT entry or entries */
    avail = 32 - gatbit;
    if (ngranules > avail)
    {
        /* Mark bits in the first GAT entry */
        gatmask = 0xffffffff << gatbit;
        assert((gran->gat[gatidx] & gatmask) == 0);

        gran->gat[gatidx] |= gatmask;
        ngranules -= avail;

        /* Mark bits in the second GAT entry */
        gatmask = 0xffffffff >> (32 - ngranules);
        assert((gran->gat[gatidx + 1] & gatmask) == 0);

        gran->gat[gatidx + 1] |= gatmask;
    }

    /* Handle the case where where all of the granules come from one entry */
    else
    {
        /* Mark bits in a single GAT entry */
        gatmask   = 0xffffffff >> (32 - ngranules);
        gatmask <<= gatbit;
        assert((gran->gat[gatidx] & gatmask) == 0);

        gran->gat[gatidx] |= gatmask;
        return;
    }
}

/****************************************************************************
 * Name: gran_free
 *
 * Description:
 *   Return memory to the granule heap.
 *
 * Input Parameters:
 *   handle - The handle previously returned by gran_initialize
 *   memory - A pointer to memory previoiusly allocated by gran_alloc.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void gran_free(struct mm_gran *gran, void *memory, size_t size)
{
    unsigned int granno;
    unsigned int gatidx;
    unsigned int gatbit;
    unsigned int granmask;
    unsigned int ngranules;
    unsigned int avail;
    uint32_t     gatmask;
    int          ret;

    assert(gran != NULL && memory && size <= 32 * (1 << gran->log2gran));

    /* Determine the granule number of the first granule in the allocation */
    granno = ((uintptr_t)memory - gran->heapstart) >> gran->log2gran;

    /* 
     * Determine the GAT table index and bit number associated with the
     * allocation.
     */
    gatidx = granno >> 5;
    gatbit = granno & 31;

    /* Determine the number of granules in the allocation */
    granmask =  (1 << gran->log2gran) - 1;
    ngranules = (size + granmask) >> gran->log2gran;

    /* Clear bits in the GAT entry or entries */
    avail = 32 - gatbit;
    if (ngranules > avail)
    {
        /* Clear bits in the first GAT entry */
        gatmask = (0xffffffff << gatbit);
        assert((gran->gat[gatidx] & gatmask) == gatmask);

        gran->gat[gatidx] &= ~gatmask;
        ngranules -= avail;

        /* Clear bits in the second GAT entry */
        gatmask = 0xffffffff >> (32 - ngranules);
        assert((gran->gat[gatidx + 1] & gatmask) == gatmask);

        gran->gat[gatidx + 1] &= ~gatmask;
    }
    /* Handle the case where where all of the granules came from one entry */
    else
    {
        /* Clear bits in a single GAT entry */
        gatmask   = 0xffffffff >> (32 - ngranules);
        gatmask <<= gatbit;
        assert((gran->gat[gatidx] & gatmask) == gatmask);

        gran->gat[gatidx] &= ~gatmask;
    }
}

#endif
