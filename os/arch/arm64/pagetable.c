#include <chinos/config.h>

#include <kernel.h>
#include <k_debug.h>
#include <k_assert.h>
#include <k_stdio.h>
#include <k_stddef.h>
#include <uapi/util.h>
#include <uart.h>
#include <instructionset.h>
#include <pgtable-prot.h>
#include <page-def.h>
#include <barrier.h>

#include <pagetable.h>

//#define KERNEL_PAGETABLE_DEBUG
#ifdef KERNEL_PAGETABLE_DEBUG
#define pagetable_dbg kdbg
#define pagetable_err kerr
#define pagetable_warn kwarn
#define pagetable_info kinfo
#else
#define pagetable_dbg(fmt, ...)
#define pagetable_err(fmt, ...)
#define pagetable_warn(fmt, ...)
#define pagetable_info(fmt, ...)
#endif

static int pte_map(pmd_t *pmd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pg_alloc)(u64))
{
    int ret = 0;
    pmd_t *pmd = pmd_p;
    pte_t *pte;
    u64 next, idx;

    if (!pmd_val(*pmd)) {
        // PMD页表中不存在PTE,需要重新分配PTE
        u64 pte_paddr = pg_alloc(PAGE_SIZE);
        assert(pte_paddr);
        // 把PTE添加到PMD页表描符中去
        *pmd = __pmd(pte_paddr | PMD_TYPE_TABLE);
        pte = (pte_t *)pte_paddr;
        pagetable_dbg("non-exist pte = %p, pmd = %p\n", pte, *pmd);
    } else {
        pagetable_dbg("exist pte %p\n", pmd_val(*pmd));
        // 存在PTE
        pte = (pte_t *)(pmd_val(*pmd) & PTE_ADDR_MASK);
        pagetable_dbg("exist pte %p\n", pte);
    }

    // 获取PTE索引
    idx = (vaddr >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
    // 更新PTE起始地址
    pte += idx;

    pagetable_dbg("pte_map:idx = %d, pte = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pte, vaddr, paddr, end);

    for (next = vaddr; next != end; pte++) {
        next = (next + PAGE_SIZE) & PAGE_MASK;
        next = next < end ? next : end;
        *pte = __pte(((paddr >> PAGE_SHIFT) << PAGE_SHIFT) | PTE_TYPE_PAGE | attr);
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}

static int pmd_map(pud_t *pud_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pg_alloc)(u64))
{
    int ret = 0;
    pud_t *pud = pud_p;
    pmd_t *pmd;
    u64 next, idx;

    if (!pud_val(*pud)) {
        // PUD页表中不存在PMU,需要重新分配PMD
        u64 pmd_paddr = pg_alloc(PAGE_SIZE);
        assert(pmd_paddr);
        // 把PMD添加到PUD页表描符中去
        *pud = __pud(pmd_paddr | PMD_TYPE_TABLE);
        pmd = (pmd_t *)pmd_paddr;
        pagetable_dbg("pmd = %p, pud = %p\n", pmd, *pud);
    } else {
        pagetable_dbg("exist pmd %p\n", pud_val(*pud));
        // 存在PMD
        pmd = (pmd_t *)(pud_val(*pud) & PTE_ADDR_MASK);
        pagetable_dbg("exist pmd %p\n", pmd);
    }

    // 获取PMD索引
    
    idx = (vaddr >> PMD_SHIFT) & (PTRS_PER_PMD - 1);
    // 更新PMD起始地址
    pmd += idx;

    pagetable_dbg("pmd_map:idx = %d, pmd = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pmd, vaddr, paddr, end);

    for (next = vaddr; next != end; pmd++) {
        next = (next + PMD_SIZE) & PMD_MASK;
        next = next < end ? next : end;

        if ((vaddr | next | paddr) & ~SECTION_MASK) {
            // 使用页式映射
            pagetable_dbg("page map: \n");
            pte_map(pmd, vaddr, next, paddr, attr, pg_alloc);
        } else {
            // 如果地址全部段对齐,使用段氏映射
            pagetable_dbg("section map: va: %p, pa:%p, next:%p\n", vaddr, paddr, next);
            *pmd = __pmd(((paddr >> PMD_SHIFT) << PMD_SHIFT) | PMD_TYPE_SECT | (attr & ~PTE_TABLE_BIT));
        }
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;

}

#if CONFIG_PGTABLE_LEVELS > 3
static int pud_map(pgd_t *pgd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pg_alloc)(u64))
{
    int ret = 0;
    pgd_t *pgd = pgd_p;
    pud_t *pud;
    u64 next, idx;

    if (!pgd_val(*pgd)) {
        // PGD页表中不存在PUD,需要重新分配PUD
        u64 pud_paddr = pg_alloc(PAGE_SIZE);
        assert(pud_paddr);
        // 把PUD添加到PGD页表描符中去
        *pgd = __pgd(pud_paddr | PUD_TYPE_TABLE);
        pud = (pud_t *)pud_paddr;
    } else {
        // 存在PUD
        pud = (pud_t *)(pgd_val(*pgd) & PTE_ADDR_MASK);
    }

    // 获取PUD索引
    idx = (vaddr >> PUD_SHIFT) & (PTRS_PER_PUD - 1);
    // 更新PUD起始地址
    pud += idx;

    pagetable_dbg("pud_map:idx = %d, pud = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pud, vaddr, paddr, end);

    for (next = vaddr; next != end; pud++) {
        next = (next + PUD_SIZE) & PUD_MASK;
        next = next < end ? next : end;
        pmd_map(pud, vaddr, next, paddr, attr, pg_alloc);
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}
#endif

int pg_map(pgd_t *pgd_p, u64 vaddr, u64 paddr, u64 size, u64 attr, u64 (*pg_alloc)(u64))
{
    int ret = 0;
    u64 next, end, idx;
    pgd_t *pgd = pgd_p;

    assert(pgd_p);
    assert(!(vaddr & lowbitsmask(PAGE_SHIFT)));
    assert(!(paddr & lowbitsmask(PAGE_SHIFT)));
    assert(size);
    assert(size <= bitmask(VA_BITS));

    // 获取PGD索引
    idx = (vaddr >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);

    // 更新PGD起始地址
    pgd += idx;

    // 对齐结束地址
    end = align_to(vaddr + size, PAGE_SIZE);

    pagetable_dbg("pg_map:idx = %d, pgd = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pgd, vaddr, paddr, end);

    for (next = vaddr; next != end; pgd++) {
        next = (next + PGDIR_SIZE) & PGDIR_MASK;
        next = next < end ? next : end;
#if CONFIG_PGTABLE_LEVELS > 3
        pud_map(pgd, vaddr, next, paddr, attr, pg_alloc);
#else
        pmd_map((pud_t *)pgd, vaddr, next, paddr, attr, pg_alloc);
#endif
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}

void dump_pgtable_brief(void)
{

}

void dump_pgtable_verbose(pgd_t *pgdp)
{
    pgd_t *pgd = pgdp;
    pud_t *pud = NULL;
    pmd_t *pmd = NULL;
    pte_t *pte = NULL;
    u64  i, j, k, l;

    _kprintf("{\n");

    int pgd_flag = 1;
    int pud_flag = 1;
    int pmd_flag = 1;
    int pte_flag = 1;

    for (i = 0; i < PTRS_PER_PGD; i++, pgd++)
    {
        if (pgd_val(*pgd)) {
            if (pgd_flag) {
                pgd_flag = 0;
                _kprintf("   \"pgd\": [\n");
            }
            _kprintf("       {\n");
            _kprintf("           \"idx\": \"%ld\",\n", i);
            _kprintf("           \"addr\": \"%p\",\n", pgd);
            _kprintf("           \"value\": \"0x%lx\",\n", pgd_val(*pgd));
            //_kprintf("pgd %p [%ld] = 0x%lx\n", pgd, i, pgd_val(*pgd));
#if CONFIG_PGTABLE_LEVELS > 3
            pud = (pud_t *)(pgd_val(*pgd) & PTE_ADDR_MASK );
            for (j = 0; j < PTRS_PER_PUD; j++, pud++) 
            {
                if (pud_val(*pud)) {
                    if (pud_flag) {
                        pud_flag = 0;
                        _kprintf("           \"pud\": [\n");
                    }
                    _kprintf("               {\n");
                    _kprintf("                   \"idx\": \"%ld\",\n", j);
                    _kprintf("                   \"addr\": \"%p\",\n", pud);
                    _kprintf("                   \"value\": \"0x%lx\",\n", pud_val(*pud));
                    //_kprintf("---->pud %p [%ld] = 0x%lx\n", pud, j, pud_val(*pud));
                    pmd = (pmd_t *)(pud_val(*pud)  & PTE_ADDR_MASK);
#else
                    pmd = (pmd_t *)(pgd_val(*pgd) & PTE_ADDR_MASK );
#endif
                    for (k = 0; k < PTRS_PER_PMD; k++, pmd++) 
                    {
                        if (pmd_val(*pmd)) {
                            if (pmd_flag) {
                                pmd_flag = 0;
                                _kprintf("                   \"pmd\": [\n");
                            }
                            _kprintf("                       {\n");
                            _kprintf("                           \"idx\": \"%ld\",\n", k);
                            _kprintf("                           \"addr\": \"%p\",\n", pmd);
                            _kprintf("                           \"value\": \"0x%lx\",\n", pmd_val(*pmd));
                            //_kprintf("-------->pmd %p [%ld] = 0x%lx\n", pmd, k, pmd_val(*pmd));
                            pte = (pte_t *)(pmd_val(*pmd) );
                            if (((u64)pte &  PTE_TYPE_PAGE) == PTE_TYPE_PAGE) {
                                pte = (pte_t *)(pmd_val(*pmd) & PTE_ADDR_MASK );
                                for (l = 0; l < PTRS_PER_PTE; l++, pte++) 
                                {
                                    if (pte_val(*pte)) {
                                        if (pte_flag) {
                                            pte_flag = 0;
                                            _kprintf("                           \"pte\": [\n");
                                        }
                                        _kprintf("                               {\n");
                                        _kprintf("                                   \"idx\": \"%ld\",\n", l);
                                        _kprintf("                                   \"addr\": \"%p\",\n", pte);
                                        _kprintf("                                   \"value\": \"0x%lx\",\n", pte_val(*pte));
                                        _kprintf("                               },\n");
                                        //_kprintf("------------>pte %p [%ld] = 0x%lx\n", pte, l, pte_val(*pte));
                                    }
                                }
                                _kprintf("                           ],\n");
                                pte_flag = 1;
                            }
                            _kprintf("                       },\n");
                        }
                    }
#if CONFIG_PGTABLE_LEVELS > 3
                    _kprintf("                   ],\n");
                    _kprintf("               },\n");
                    pmd_flag = 1;
                }
            }
#endif
            _kprintf("           ],\n");
            _kprintf("       },\n");
            pud_flag = 1;
        }
    }
    _kprintf("    ],\n");
    _kprintf("}\n");
}

