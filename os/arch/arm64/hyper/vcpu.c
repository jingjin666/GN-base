#include <chinos/config.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT

#include <kernel.h>
#include <k_stdio.h>
#include <k_string.h>
#include <k_stdlib.h>
#include <uapi/util.h>
#include <board.h>
#include <irq.h>
#include <cpu.h>
#include <tlb.h>
#include <instructionset.h>
#include <barrier.h>
#include <pagetable.h>
#include <gran.h>
#include <init.h>
#include <task.h>
#include <scheduler.h>
#include <mmap.h>
#include <mm_heap.h>
#include <sysreg.h>

#include "vcpu.h"

void vcpu_reg_save(vcpu_t *vcpu)
{
    vcpu->regs[VCPU_REG_SCTLR] =    read_sysreg(SCTLR_EL1);
    vcpu->regs[VCPU_REG_TTBR0] =    read_sysreg(TTBR0_EL1);
    vcpu->regs[VCPU_REG_TTBR1] =    read_sysreg(TTBR1_EL1);
    vcpu->regs[VCPU_REG_TCR] =      read_sysreg(TCR_EL1);
    vcpu->regs[VCPU_REG_MAIR] =     read_sysreg(MAIR_EL1);
    vcpu->regs[VCPU_REG_AMAIR] =    read_sysreg(AMAIR_EL1);
    vcpu->regs[VCPU_REG_CONTEXTIDR] =     read_sysreg(CONTEXTIDR_EL1);
    vcpu->regs[VCPU_REG_ACTLR] =    read_sysreg(ACTLR_EL1);
    vcpu->regs[VCPU_REG_CPACR] =    read_sysreg(CPACR_EL1);
    vcpu->regs[VCPU_REG_AFSR0] =    read_sysreg(AFSR0_EL1);
    vcpu->regs[VCPU_REG_AFSR1] =    read_sysreg(AFSR1_EL1);
    vcpu->regs[VCPU_REG_ESR] =      read_sysreg(ESR_EL1);
    vcpu->regs[VCPU_REG_FAR] =      read_sysreg(FAR_EL1);
    vcpu->regs[VCPU_REG_ISR] =      read_sysreg(ISR_EL1);
    vcpu->regs[VCPU_REG_VBAR] =     read_sysreg(VBAR_EL1);
    vcpu->regs[VCPU_REG_TPIDR_EL1] =read_sysreg(TPIDR_EL1);
    vcpu->regs[VCPU_REG_SP_EL1] =   read_sysreg(SP_EL1);
    vcpu->regs[VCPU_REG_ELR_EL1] =  read_sysreg(ELR_EL1);
    vcpu->regs[VCPU_REG_SPSR_EL1] = read_sysreg(SPSR_EL1);

    // save vtimer
    vcpu->regs[VCPU_REG_CNTV_CTL] =     read_sysreg(CNTV_CTL_EL0);
    vcpu->regs[VCPU_REG_CNTV_CVAL] =    read_sysreg(CNTV_CVAL_EL0);
    vcpu->regs[VCPU_REG_CNTVOFF] =      read_sysreg(CNTVOFF_EL2);
    vcpu->regs[VCPU_REG_CNTKCTL_EL1] =  read_sysreg(CNTKCTL_EL1);

    // save vgic
    vcpu->vgic.hcr = read_sysreg_s(SYS_ICH_HCR_EL2);
    vcpu->vgic.vmcr = read_sysreg_s(SYS_ICH_VMCR_EL2);
    vcpu->vgic.apr = read_sysreg_s(SYS_ICH_AP0R0_EL2);
    vcpu->vgic.lr[0] = read_sysreg_s(SYS_ICH_LR0_EL2);
    vcpu->vgic.lr[1] = read_sysreg_s(SYS_ICH_LR1_EL2);
    vcpu->vgic.lr[2] = read_sysreg_s(SYS_ICH_LR2_EL2);
    vcpu->vgic.lr[3] = read_sysreg_s(SYS_ICH_LR3_EL2);
    vcpu->vgic.lr[4] = read_sysreg_s(SYS_ICH_LR4_EL2);
    vcpu->vgic.lr[5] = read_sysreg_s(SYS_ICH_LR5_EL2);
    vcpu->vgic.lr[6] = read_sysreg_s(SYS_ICH_LR6_EL2);
    vcpu->vgic.lr[7] = read_sysreg_s(SYS_ICH_LR7_EL2);
    vcpu->vgic.lr[8] = read_sysreg_s(SYS_ICH_LR8_EL2);
    vcpu->vgic.lr[9] = read_sysreg_s(SYS_ICH_LR9_EL2);
    vcpu->vgic.lr[10] = read_sysreg_s(SYS_ICH_LR10_EL2);
    vcpu->vgic.lr[11] = read_sysreg_s(SYS_ICH_LR11_EL2);
    vcpu->vgic.lr[12] = read_sysreg_s(SYS_ICH_LR12_EL2);
    vcpu->vgic.lr[13] = read_sysreg_s(SYS_ICH_LR13_EL2);
    vcpu->vgic.lr[14] = read_sysreg_s(SYS_ICH_LR14_EL2);
    vcpu->vgic.lr[15] = read_sysreg_s(SYS_ICH_LR15_EL2);
}

void vcpu_reg_restore(vcpu_t *vcpu)
{
    write_sysreg(vcpu->regs[VCPU_REG_SCTLR],    SCTLR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_TTBR0],    TTBR0_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_TTBR1],    TTBR1_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_TCR],      TCR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_MAIR],     MAIR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_AMAIR],    AMAIR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_CONTEXTIDR],     CONTEXTIDR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_CPACR],    CPACR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_AFSR0],    AFSR0_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_AFSR1],    AFSR1_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_ESR],      ESR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_FAR],      FAR_EL1);

    write_sysreg(vcpu->regs[VCPU_REG_VBAR],     VBAR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_TPIDR_EL1],TPIDR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_SP_EL1],   SP_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_ELR_EL1],  ELR_EL1);
    write_sysreg(vcpu->regs[VCPU_REG_SPSR_EL1], SPSR_EL1);

    // store vtimer
    write_sysreg(vcpu->regs[VCPU_REG_CNTV_CTL],     CNTV_CVAL_EL0);
    write_sysreg(vcpu->regs[VCPU_REG_CNTV_CVAL],    CNTV_CVAL_EL0);
    write_sysreg(vcpu->regs[VCPU_REG_CNTVOFF],      CNTVOFF_EL2);
    write_sysreg(vcpu->regs[VCPU_REG_CNTKCTL_EL1],  CNTKCTL_EL1);

    // store vgic
    write_sysreg_s(vcpu->vgic.hcr, SYS_ICH_HCR_EL2);
    write_sysreg_s(vcpu->vgic.vmcr, SYS_ICH_VMCR_EL2);
    write_sysreg_s(vcpu->vgic.apr, SYS_ICH_AP0R0_EL2);
    write_sysreg_s(vcpu->vgic.lr[0], SYS_ICH_LR0_EL2);
    write_sysreg_s(vcpu->vgic.lr[1], SYS_ICH_LR1_EL2);
    write_sysreg_s(vcpu->vgic.lr[2], SYS_ICH_LR2_EL2);
    write_sysreg_s(vcpu->vgic.lr[3], SYS_ICH_LR3_EL2);
    write_sysreg_s(vcpu->vgic.lr[4], SYS_ICH_LR4_EL2);
    write_sysreg_s(vcpu->vgic.lr[5], SYS_ICH_LR5_EL2);
    write_sysreg_s(vcpu->vgic.lr[6], SYS_ICH_LR6_EL2);
    write_sysreg_s(vcpu->vgic.lr[7], SYS_ICH_LR7_EL2);
    write_sysreg_s(vcpu->vgic.lr[8], SYS_ICH_LR8_EL2);
    write_sysreg_s(vcpu->vgic.lr[9], SYS_ICH_LR9_EL2);
    write_sysreg_s(vcpu->vgic.lr[10], SYS_ICH_LR10_EL2);
    write_sysreg_s(vcpu->vgic.lr[11], SYS_ICH_LR11_EL2);
    write_sysreg_s(vcpu->vgic.lr[12], SYS_ICH_LR12_EL2);
    write_sysreg_s(vcpu->vgic.lr[13], SYS_ICH_LR13_EL2);
    write_sysreg_s(vcpu->vgic.lr[14], SYS_ICH_LR14_EL2);
    write_sysreg_s(vcpu->vgic.lr[15], SYS_ICH_LR15_EL2);  
}

void vcpu_store(void)
{
    struct tcb *current = this_task();

    vcpu_reg_save(&current->vcpu);
}


void vcpu_restore(void)
{
    struct tcb *current = this_task();

    vcpu_reg_restore(&current->vcpu);
}

#endif
