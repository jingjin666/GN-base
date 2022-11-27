#include <chinos/config.h>
#include <registerset.h>

.global vector

.macro ventry label
.align 7
    b       \label
.endm

.macro set_irq_stack
    // set irq stack to run c
    // 中断处理使用单独的中断栈
    ldr     x0, = irq_stack
    ldr     x1, = CONFIG_DEFAULT_TASK_STACKSIZE
    add     x0, x0, x1
    mov     sp, x0
.endm

.macro store_context
    // store context to sp_el1(tcb->context->regs)
    // 保存X0~X29
    stp     x0,  x1,  [sp, #16 * 0]
    stp     x2,  x3,  [sp, #16 * 1]
    stp     x4,  x5,  [sp, #16 * 2]
    stp     x6,  x7,  [sp, #16 * 3]
    stp     x8,  x9,  [sp, #16 * 4]
    stp     x10, x11, [sp, #16 * 5]
    stp     x12, x13, [sp, #16 * 6]
    stp     x14, x15, [sp, #16 * 7]
    stp     x16, x17, [sp, #16 * 8]
    stp     x18, x19, [sp, #16 * 9]
    stp     x20, x21, [sp, #16 * 10]
    stp     x22, x23, [sp, #16 * 11]
    stp     x24, x25, [sp, #16 * 12]
    stp     x26, x27, [sp, #16 * 13]
    stp     x28, x29, [sp, #16 * 14]

    // 获取ELR
    mrs     x9, elr_el1
    // 获取SP
    mrs     x10, sp_el0
    // 获取SPSR
    mrs     x11, spsr_el1

    // 保存lr(x30),ELR
    stp     lr, x9, [sp, #CTX_OFFS_LR]

    // 保存sp,SPSR
    stp     x10, x11, [sp, #CTX_OFFS_SP]
.endm

.section ".vector", "ax"
.align 11

vector:
    ventry  invalid_vector_entry           // Synchronous EL1t/EL2t
    ventry  invalid_vector_entry           // IRQ EL1t/EL2t
    ventry  invalid_vector_entry           // FIQ EL1t/EL2t
    ventry  invalid_vector_entry           // SError EL1t/EL2t

    ventry  cur_el_sync                    // Current EL Synchronous (EL1/2)
    ventry  cur_el_irq                     // IRQ
    ventry  invalid_vector_entry           // FIQ
    ventry  cur_el_serr                    // SError

    ventry  lower_el_sync                  // Synchronous 64-bit EL0/EL1
    ventry  lower_el_irq                   // IRQ 64-bit EL0/EL1
    ventry  invalid_vector_entry           // FIQ 64-bit EL0/EL1
    ventry  lower_el_serr                  // SError 64-bit EL0/EL1

    ventry  invalid_vector_entry           // Synchronous 32-bit EL0/EL1
    ventry  invalid_vector_entry           // IRQ 32-bit EL0/EL1
    ventry  invalid_vector_entry           // FIQ 32-bit EL0/EL1
    ventry  invalid_vector_entry           // SError 32-bit EL0/EL1

invalid_vector_entry:
    b   .

cur_el_sync:
    bl      cel_sync_traps
    eret
    b   .

cur_el_irq:
    store_context

    set_irq_stack

    b       decode_irq
    b .

cur_el_serr:
    bl      cel_serr_traps
    eret
    b   .

lower_el_sync:
    store_context

    set_irq_stack

    bl      lel_sync_traps
    eret
    b .

lower_el_irq:
    store_context

    set_irq_stack

    b       decode_irq
    b .

lower_el_serr:
    bl      lel_serr_traps
    eret
    b .

.section ".stack", "wa"
irq_stack:
    .space CONFIG_DEFAULT_TASK_STACKSIZE
