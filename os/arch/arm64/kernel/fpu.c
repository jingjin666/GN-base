#include <chinos/config.h>

#include <kernel.h>
#include <k_stdbool.h>
#include <fpu.h>

#ifdef CONFIG_ARCH_FPU

static BOOTPHYSIC bool fp_simd_init(void)
{
    enable_fpu_EL01();
    return true;
}

BOOTPHYSIC void fpu_init(void)
{
    fp_simd_init();
}

#endif
