#include <chinos/config.h>
#include <system.h>

// 重新命名x16寄存器为临时寄存器
tmp     .req    x16

.global _start

.section ".entry", "ax"
_start:
    // 当前异常等级
    mrs     tmp, CurrentEL
    and     tmp, tmp, #0b1100
    
    cmp     tmp, #(1 << 2)
    // 跳转到EL1处理
    beq     el1_entry

    cmp     tmp, #(2 << 2)
    // 跳转到EL2处理
    beq     switch_to_el1_from_el2

    cmp     tmp, #(3 << 2)
    // 跳转到EL3处理
    beq     switch_to_el2_from_el3

switch_to_el2_from_el3:
    // 关闭EL3的指令和数据缓存和MMU
    mrs     tmp, sctlr_el3
    bic     tmp, tmp, #SCTLR_ELx_I
    bic     tmp, tmp, #SCTLR_ELx_C
    bic     tmp, tmp, #SCTLR_ELx_M
    msr     sctlr_el3, tmp

    // 从EL3切换到EL2
	/* Return to the EL2_SP2 mode from EL3 */
	ldr	    tmp, =(SPSR_EL_DEBUG_MASK | SPSR_EL_SERR_MASK |\
			    SPSR_EL_IRQ_MASK | SPSR_EL_FIQ_MASK |\
			    SPSR_EL_M_AARCH64 | SPSR_EL_M_EL2H)
	msr	    spsr_el3, tmp
	adr     tmp, switch_to_el1_from_el2
	msr	    elr_el3, tmp
	eret

switch_to_el1_from_el2:
    // 关闭EL2的指令和数据缓存和MMU
    mrs     tmp, sctlr_el2
    bic     tmp, tmp, #SCTLR_ELx_I
    bic     tmp, tmp, #SCTLR_ELx_C
    bic     tmp, tmp, #SCTLR_ELx_M
    msr     sctlr_el2, tmp

    // 从EL2切换到EL1
    /* Initialize HCR_EL2 */
	ldr	    tmp, =(HCR_EL2_RW_AARCH64 | HCR_EL2_HCD_DIS)
	msr	    hcr_el2, tmp

	/* Return to the EL1_SP1 mode from EL2 */
	ldr	    tmp, =(SPSR_EL_DEBUG_MASK | SPSR_EL_SERR_MASK |\
			    SPSR_EL_IRQ_MASK | SPSR_EL_FIQ_MASK |\
			    SPSR_EL_M_AARCH64 | SPSR_EL_M_EL1H)
	msr	    spsr_el2, tmp
	adr     tmp, el1_entry
	msr	    elr_el2, tmp
	eret

el1_entry:
    // 关闭EL1的指令和数据缓存和MMU
    mrs     tmp, sctlr_el1
    bic     tmp, tmp, #SCTLR_ELx_I
    bic     tmp, tmp, #SCTLR_ELx_C
    bic     tmp, tmp, #SCTLR_ELx_M
    msr     sctlr_el1, tmp

cache_disable_done:
    dsb     sy
    isb

    // MMU前设置C堆栈,注意此刻SP必须为物理地址
    adr     x1, boot_stack
    mov     sp, x1

    // 使能浮点单元
#ifdef CONFIG_ARCH_FPU
    bl      fpu_init
#endif

    // MMU前的初始化工作
    bl      boot_setup_mmu

    // 打开MMU
    mrs     tmp, sctlr_el1
    orr     tmp, tmp, #SCTLR_ELx_M
    msr     sctlr_el1, tmp

    // 设置MMU后设置C堆栈,注意此刻SP为虚拟地址
    ldr     x0, = idle_stack
    ldr     x1, = CONFIG_IDLE_TASK_STACKSIZE
    add     x0, x0, x1
    mov     sp, x0

    // 正式进入虚拟地址空间
    ldr     x2, = mmu_enabled
    br      x2

mmu_enabled:
    b init_kernel

.global idle_stack

.section ".stack", "wa"
idle_stack:
    .space CONFIG_IDLE_TASK_STACKSIZE
