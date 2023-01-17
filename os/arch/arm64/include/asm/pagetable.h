#ifndef __ASM_PAGETABLE_H
#define __ASM_PAGETABLE_H

#ifndef __ASSEMBLY__

#include <chinos/config.h>

#include <kernel.h>
#include <k_types.h>
#include <page-def.h>
#include <pgtable-prot.h>
#include <uapi/util.h>
#include <board.h>

/* PGD */
/* 每个ENTRY包含512G内存区域 */
typedef struct page_table
{
    unsigned long entry[PTRS_PER_PGD];
} aligned_data(PAGE_SIZE) pgtable_pgd_t;

#if CONFIG_PGTABLE_LEVELS > 3
/* PUD */
/* 每个ENTRY包含1G内存区域 */
typedef struct pgtable_pud
{
    unsigned long entry[PTRS_PER_PUD];
} aligned_data(PAGE_SIZE) pgtable_pud_t;
#endif

#if CONFIG_PGTABLE_LEVELS > 2
/* PMD */
/* 每个ENTRY包含2M内存区域 */
typedef struct pgtable_pmd
{
    unsigned long entry[PTRS_PER_PMD];
} aligned_data(PAGE_SIZE) pgtable_pmd_t;
#endif

/* PTE */
/* 每个ENTRY包含4K内存区域 */
typedef struct pgtable_pte
{
    unsigned long entry[PTRS_PER_PTE];
} aligned_data(PAGE_SIZE) pgtable_pte_t;

static inline unsigned long vbase_to_pbase(unsigned long vbase)
{
    return vbase - RAM_VBASE + RAM_PBASE;
}

static inline unsigned long pbase_to_vbase(unsigned long pbase)
{
    return pbase - RAM_PBASE + RAM_VBASE;
}

int idmap_pt_map(pgd_t *pgd_p, unsigned long vaddr, unsigned long paddr, unsigned long size, unsigned long attr, u64 (*pt_alloc)(unsigned long));
int pt_map(pgd_t *pgd_p, unsigned long vaddr, unsigned long paddr, unsigned long size, unsigned long attr, u64 (*pt_alloc)(unsigned long));
unsigned long pgd_addressing(unsigned long *pgd, unsigned long va);

#endif /* !__ASSEMBLY__ */

#endif
