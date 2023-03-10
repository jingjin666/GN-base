#include <chinos/config.h>

#include <k_assert.h>
#include <instructionset.h>
#include <barrier.h>
#include <tlb.h>
#include <uapi/util.h>

#include <cache.h>

#define LOUU(x)    (((x) >> 27)        & bitmask(3))
#define LOC(x)     (((x) >> 24)        & bitmask(3))
#define LOUIS(x)   (((x) >> 21)        & bitmask(3))
#define CTYPE(x,n) (((x) >> (n*3))     & bitmask(3))

#define LINEBITS(s)     (((s) & bitmask(3)) + 4)
#define ASSOC(s)        ((((s) >> 3) & bitmask(10)) + 1)
#define NSETS(s)        ((((s) >> 13) & bitmask(15)) + 1)

enum arm_cache_type {
    ARMCacheI =    1,
    ARMCacheD =    2,
    ARMCacheID =   3,
};

static unsigned long readCacheSize(int level, char instruction)
{
    unsigned long size, csselr_old;
    /* Save CSSELR */
    MRS("csselr_el1", csselr_old);
    /* Select cache level */
    MSR("csselr_el1", ((level << 1) | instruction));
    /* Read 'size' */
    MRS("ccsidr_el1", size);
    /* Restore CSSELR */
    MSR("csselr_el1", csselr_old);
    return size;
}

static void cleanInvalidate_D_by_level(int l)
{
    unsigned long lsize = readCacheSize(l, 0);
    int lbits = LINEBITS(lsize);
    int assoc = ASSOC(lsize);
    int assoc_bits = BITS_PER_LONG - clzl(assoc - 1);
    int nsets = NSETS(lsize);

    for (int w = 0; w < assoc; w++) {
        for (int s = 0; s < nsets; s++) {
            cleanInvalidateByWSL((w << (32 - assoc_bits)) |
                                 (s << lbits) | (l << 1));
        }
    }
}

void clean_D_PoU(void)
{
    int clid = readCLID();
    int lou = LOUU(clid);

    for (int l = 0; l < lou; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            unsigned long lsize = readCacheSize(l, 0);
            int lbits = LINEBITS(lsize);
            int assoc = ASSOC(lsize);
            int assoc_bits = BITS_PER_LONG - clzl(assoc - 1);
            int nsets = NSETS(lsize);
            for (int w = 0; w < assoc; w++) {
                for (int s = 0; s < nsets; s++) {
                    cleanByWSL((w << (32 - assoc_bits)) |
                               (s << lbits) | (l << 1));
                }
            }
        }
    }
}

void cleanInvalidate_D_PoC(void)
{
    int clid = readCLID();
    int loc = LOC(clid);

    for (int l = 0; l < loc; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            cleanInvalidate_D_by_level(l);
        }
    }
}

void cleanInvalidate_L1D(void)
{
    cleanInvalidate_D_by_level(0);
}

void cleanInvalidateL1Caches(void)
{
    dsb();
    cleanInvalidate_D_PoC();
    dsb();
    invalidate_I_PoU();
    dsb();
}

void cleanCaches_PoU(void)
{
    dsb();
    clean_D_PoU();
    dsb();
    invalidate_I_PoU();
    dsb();
}

void cleanInvalidateCaches(void)
{
    cleanCaches_PoU();
    cleanInvalidateL1Caches();
    isb();
}

