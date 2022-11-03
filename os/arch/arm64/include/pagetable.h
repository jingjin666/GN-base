#ifndef __PAGETABLE_H
#define __PAGETABLE_H

#include <chinos/config.h>

#include <kernel.h>
#include <k_stddef.h>
#include <k_stdint.h>
#include <page-def.h>
#include <pgtable-prot.h>
#include <uapi/util.h>

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

int pg_map(pgd_t *pgd_p, u64 vaddr, u64 paddr, u64 size, u64 attr, u64 (*pg_alloc)(u64));

#endif
