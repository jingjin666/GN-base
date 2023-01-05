#include <chinos/config.h>

#include <kernel.h>
#include <k_debug.h>
#include <k_assert.h>
#include <k_stdio.h>
#include <k_stddef.h>
#include <uapi/util.h>
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

static int idmap_pte_map(pmd_t *pmd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pmd_t *pmd = pmd_p;
    pte_t *pte;
    u64 next, idx;

    if (!pmd_val(*pmd)) {
        // PMD页表中不存在PTE,需要重新分配PTE
        u64 pte_paddr = pt_alloc(PAGE_SIZE);
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

static int idmap_pmd_map(pud_t *pud_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pud_t *pud = pud_p;
    pmd_t *pmd;
    u64 next, idx;

    if (!pud_val(*pud)) {
        // PUD页表中不存在PMU,需要重新分配PMD
        u64 pmd_paddr = pt_alloc(PAGE_SIZE);
        assert(pmd_paddr);
        // 把PMD添加到PUD页表描符中去
        *pud = __pud(pmd_paddr | PMD_TYPE_TABLE);
        pmd = (pmd_t *)pmd_paddr;
        pagetable_dbg("non-exist pmd = %p, pud = %p\n", pmd, *pud);
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
            idmap_pte_map(pmd, vaddr, next, paddr, attr, pt_alloc);
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
static int idmap_pud_map(pgd_t *pgd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pgd_t *pgd = pgd_p;
    pud_t *pud;
    u64 next, idx;

    if (!pgd_val(*pgd)) {
        // PGD页表中不存在PUD,需要重新分配PUD
        u64 pud_paddr = pt_alloc(PAGE_SIZE);
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
        idmap_pmd_map(pud, vaddr, next, paddr, attr, pt_alloc);
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}
#endif

int idmap_pt_map(pgd_t *pgd_p, u64 vaddr, u64 paddr, u64 size, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    u64 next, end, idx;
    pgd_t *pgd = pgd_p;

    assert(pgd_p);
    assert(!(vaddr & bitmask(PAGE_SHIFT)));
    assert(!(paddr & bitmask(PAGE_SHIFT)));
    assert(size);
    assert(size <= bit(VA_BITS));

    // 获取PGD索引
    idx = (vaddr >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);

    // 更新PGD起始地址
    pgd += idx;

    // 对齐结束地址
    end = ALIGN(vaddr + size, PAGE_SIZE);

    pagetable_dbg("pg_map:idx = %d, pgd = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pgd, vaddr, paddr, end);

    for (next = vaddr; next != end; pgd++) {
        next = (next + PGDIR_SIZE) & PGDIR_MASK;
        next = next < end ? next : end;
#if CONFIG_PGTABLE_LEVELS > 3
        idmap_pud_map(pgd, vaddr, next, paddr, attr, pt_alloc);
#else
        idmap_pmd_map((pud_t *)pgd, vaddr, next, paddr, attr, pt_alloc);
#endif
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}

void dump_pgtable_brief(void)
{

}

void dump_pgtable_verbose(pgd_t *pgdp, char is_idmap)
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
            if (is_idmap)
                pud = (pud_t *)(pgd_val(*pgd) & PTE_ADDR_MASK);
            else
                pud = (pud_t *)pbase_to_vbase(pgd_val(*pgd) & PTE_ADDR_MASK);
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
                    if (is_idmap)
                        pmd = (pmd_t *)(pud_val(*pud)  & PTE_ADDR_MASK);
                    else
                        pmd = (pmd_t *)pbase_to_vbase(pud_val(*pud)  & PTE_ADDR_MASK);
#else
                    if (is_idmap)
                        pmd = (pmd_t *)(pgd_val(*pgd) & PTE_ADDR_MASK);
                    else
                        pmd = (pmd_t *)pbase_to_vbase(pgd_val(*pgd) & PTE_ADDR_MASK);
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
                                if (is_idmap)
                                    pte = (pte_t *)(pmd_val(*pmd) & PTE_ADDR_MASK);
                                else
                                    pte = (pte_t *)pbase_to_vbase(pmd_val(*pmd) & PTE_ADDR_MASK);
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

static int pte_map(pmd_t *pmd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pmd_t *pmd = pmd_p;
    pte_t *pte;
    u64 next, idx;

    if (!pmd_val(*pmd)) {
        // PMD页表中不存在PTE,需要重新分配PTE
        u64 pte_paddr = pt_alloc(PAGE_SIZE);
        assert(pte_paddr);
        // 把PTE添加到PMD页表描符中去
        *pmd = __pmd(vbase_to_pbase(pte_paddr | PMD_TYPE_TABLE));
        pte = (pte_t *)pte_paddr;
        pagetable_dbg("non-exist pte = %p, pmd = %p\n", pte, *pmd);
    } else {
        pagetable_dbg("exist pte %p\n", pmd_val(*pmd));
        // 存在PTE
        pte = (pte_t *)pbase_to_vbase(pmd_val(*pmd) & PTE_ADDR_MASK);
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

static int pmd_map(pud_t *pud_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pud_t *pud = pud_p;
    pmd_t *pmd;
    u64 next, idx;

    if (!pud_val(*pud)) {
        // PUD页表中不存在PMU,需要重新分配PMD
        u64 pmd_paddr = pt_alloc(PAGE_SIZE);
        assert(pmd_paddr);
        // 把PMD添加到PUD页表描符中去
        *pud = __pud(vbase_to_pbase(pmd_paddr | PMD_TYPE_TABLE));
        pmd = (pmd_t *)pmd_paddr;
        pagetable_dbg("non-exist pmd = %p, pud = %p\n", pmd, *pud);
    } else {
        pagetable_dbg("exist pmd %p\n", pud_val(*pud));
        // 存在PMD
        pmd = (pmd_t *)pbase_to_vbase(pud_val(*pud) & PTE_ADDR_MASK);
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
            pte_map(pmd, vaddr, next, paddr, attr, pt_alloc);
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
static int pud_map(pgd_t *pgd_p, u64 vaddr, u64 end, u64 paddr, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    pgd_t *pgd = pgd_p;
    pud_t *pud;
    u64 next, idx;

    if (!pgd_val(*pgd)) {
        // PGD页表中不存在PUD,需要重新分配PUD
        u64 pud_paddr = pt_alloc(PAGE_SIZE);
        assert(pud_paddr);
        // 把PUD添加到PGD页表描符中去
        *pgd = __pgd(vbase_to_pbase(pud_paddr | PUD_TYPE_TABLE));
        pud = (pud_t *)pud_paddr;
    } else {
        // 存在PUD
        pud = (pud_t *)pbase_to_vbase(pgd_val(*pgd) & PTE_ADDR_MASK);
    }

    // 获取PUD索引
    idx = (vaddr >> PUD_SHIFT) & (PTRS_PER_PUD - 1);
    // 更新PUD起始地址
    pud += idx;

    pagetable_dbg("pud_map:idx = %d, pud = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pud, vaddr, paddr, end);

    for (next = vaddr; next != end; pud++) {
        next = (next + PUD_SIZE) & PUD_MASK;
        next = next < end ? next : end;
        pmd_map(pud, vaddr, next, paddr, attr, pt_alloc);
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}
#endif

int pt_map(pgd_t *pgd_p, u64 vaddr, u64 paddr, u64 size, u64 attr, u64 (*pt_alloc)(u64))
{
    int ret = 0;
    u64 next, end, idx;
    pgd_t *pgd = pgd_p;

    assert(pgd_p);
    assert(!(vaddr & bitmask(PAGE_SHIFT)));
    assert(!(paddr & bitmask(PAGE_SHIFT)));
    assert(size);
    assert(size <= bit(VA_BITS));

    // 获取PGD索引
    idx = (vaddr >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);

    // 更新PGD起始地址
    pgd += idx;

    // 对齐结束地址
    end = ALIGN(vaddr + size, PAGE_SIZE);

    pagetable_dbg("pg_map:idx = %d, pgd = %p, vaddr = %p, paddr = %p, end = %p\n", idx, pgd, vaddr, paddr, end);

    for (next = vaddr; next != end; pgd++) {
        next = (next + PGDIR_SIZE) & PGDIR_MASK;
        next = next < end ? next : end;
#if CONFIG_PGTABLE_LEVELS > 3
        pud_map(pgd, vaddr, next, paddr, attr, pt_alloc);
#else
        pmd_map((pud_t *)pgd, vaddr, next, paddr, attr, pt_alloc);
#endif
        paddr += next - vaddr;
        vaddr = next;
    }

    return ret;
}

#define WALK_VA_MASK ((1UL << VA_BITS) - 1) // VA_MASK = 0x0000_ffff_ffff_ffff
#define WALK_PA_MASK WALK_VA_MASK
#define WALK_INDEX_MASK (PTRS_PER_PTE - 1) // INDEX_MASK = 0x1ff
#define WALK_PAGE_MASK ((1UL << PAGE_SHIFT) - 1) // PAGE_MASK = 0xfff

#if CONFIG_PGTABLE_LEVELS > 3
static unsigned long pud_addressing(unsigned long *pud, unsigned long va);
#endif
static unsigned long pmd_addressing(unsigned long *pmd, unsigned long va);
static unsigned long pte_addressing(unsigned long *pmd, unsigned long va);

unsigned long pgd_addressing(unsigned long *pgd, unsigned long va)
{
    va &= WALK_VA_MASK; // va = 0x0000_0000_1000_1000

    // 获取L0索引
    unsigned int L0_index = (va  >> PGDIR_SHIFT) & WALK_INDEX_MASK; //L0_index = 0x0000_0000_1000_1000 >> 39 & 0x1ff = 0

    // 获取PGD页表描述符
    unsigned long pgd_desc = pgd[L0_index];

    // 查看Table Descriptor Type位
    unsigned char desc_type = pgd_desc & 0x3;
    if (desc_type == 3) {
        // Type位为3代表，此描述符保存了下一级页表的地址，也就是PUD的地址，PUD的地址必须4K对齐
        unsigned long *pud = (unsigned long *)pbase_to_vbase((pgd_desc & (~ 0x3)) & PTE_ADDR_MASK);
#if CONFIG_PGTABLE_LEVELS > 3
        return pud_addressing(pud, va);
#else
        return pmd_addressing(pud, va);
#endif
    } else if(desc_type == 1) {
        // 一级页表type必须为3
        kprintf("%s:%d addressing error\n", __FUNCTION__, __LINE__);
        return -1;
    } else {
        kprintf("%s:%d addressing error\n", __FUNCTION__, __LINE__);
        return -1;
    }
}

#if CONFIG_PGTABLE_LEVELS > 3
static unsigned long pud_addressing(unsigned long *pud, unsigned long va)
{
    unsigned long pa;
	// 获取L1索引
	unsigned int L1_index = (va >> PUD_SHIFT) & WALK_INDEX_MASK; //L1_index = 0x0000_0000_1000_1000 >> 30 & 0x1ff = 0

    // 获取PUD页表描述符
	unsigned long pud_desc = pud[L1_index ];

    // 查看Table Descriptor Type位
	unsigned char desc_type = pud_desc & 0x3;
    if (desc_type == 3) {
        // Type位为3代表，此描述符保存了下一级页表的地址，也就是PMD的地址，PMD的地址必须4K对齐
        unsigned long *pmd = (unsigned long *)pbase_to_vbase((pud_desc & (~ 0x3)) & PTE_ADDR_MASK);
        return pmd_addressing(pmd, va);
    } else if(desc_type == 1) {
        // Type位为1代表，此描述符保存的物理地址，PA地址必须1G对齐
        pa = pud_desc & (~ WALK_PAGE_MASK);
        unsigned long offset = va & ((1UL << PUD_SHIFT) - 1);
        pa += offset;
    } else {
        kprintf("%s:%d addressing error\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return pa & WALK_PA_MASK;
}
#endif

static unsigned long pmd_addressing(unsigned long *pmd, unsigned long va)
{
    unsigned long pa;
	// 获取L2索引
	unsigned int L2_index = (va >> PMD_SHIFT) & WALK_INDEX_MASK; //L2_index = 0x0000_0000_1000_1000 >> 21 & 0x1ff = 0x80 & 0x1ff = 128

    // 获取PMD页表描述符
	unsigned long pmd_desc = pmd[L2_index];

    // 查看Table Descriptor Type位
	unsigned char desc_type = pmd_desc & 0x3;
    if (desc_type == 3) {
        // Type位为3代表，此描述符保存了下一级页表的地址，也就是PTE的地址，PTE的地址必须4K对齐
        unsigned long *pte = (unsigned long *)pbase_to_vbase((pmd_desc & (~ 0x3)) & PTE_ADDR_MASK);
        return pte_addressing(pte, va);
    } else if(desc_type == 1) {
        // Type位为1代表，此描述符保存的物理地址，PA地址必须2M对齐
        pa = pmd_desc & (~ WALK_PAGE_MASK);
        unsigned long offset = va & ((1UL << PMD_SHIFT) - 1);
        pa += offset;
    } else {
        kprintf("%s:%d addressing error\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return pa & WALK_PA_MASK;
}

static unsigned long pte_addressing(unsigned long *pte, unsigned long va)
{
    unsigned long pa;
	// 获取L3索引
	unsigned int L3_index = (va >> PAGE_SHIFT) & WALK_INDEX_MASK; //L3_index = 0x0000_0000_1000_1000 >> 12 & 0x1ff = 1

    // 获取PMD页表描述符
	unsigned long pte_desc = pte[L3_index];

    // 查看Table Descriptor Type位
	unsigned char desc_type = pte_desc & 0x3;
    if (desc_type == 3) {
        // Type位为3代表，此描述符保存的物理地址，PA地址必须4K对齐
        // PTE的Type位虽然为3，但并不代表还指向下一级，PTE已经是最后一级，描述的就是物理地址，当我map时把Type设置为1会出现错误，Type=1只有在PUD和PMD阶段起作用
        pa = pte_desc & (~ WALK_PAGE_MASK);
        unsigned long offset = va & ((1UL << PAGE_SHIFT) - 1);
        pa += offset;
    } else {
        kprintf("%s:%d addressing error\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return pa & WALK_PA_MASK;
}

