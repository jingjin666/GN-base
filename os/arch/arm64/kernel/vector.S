#include <chinos/config.h>
#include <registerset.h>

#ifdef CONFIG_HYPERVISOR_SUPPORT
#define ELR     elr_el2
#define SPSR    spsr_el2
#else
#define ELR     elr_el1
#define SPSR    spsr_el1
#endif

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

    // debug save elr and spsr
    mov     x0, x9
    mov     x1, x11
.endm

.macro store_context
    // 保存X0~X29
    sub     sp,  sp, #CTX_SIZE
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
    str     x30, [sp, #CTX_OFFS_LR]

    // 获取ELR
    mrs     x9, ELR
    // 保存ELR
    str     x9, [sp, #CTX_OFFS_PC]

    // 获取原来的SP，SP在之前减去了CTX_SIZE，所以此处要加上CTX_SIZE
    add     x10, sp, #CTX_SIZE
    // 获取SPSR
    mrs     x11, SPSR
    // 保存SP,SPSR
    stp     x10, x11, [sp, #CTX_OFFS_SP]

    
    //add     sp, sp, #CTX_SIZE
    mov     x0, sp
.endm

.macro restore_context
    // restore PC -> ELR_ELx
    ldr     x9, [sp, #CTX_OFFS_PC]
    msr     ELR, x9

    // restore SP -> SP_ELx, PSTATE -> SPSR_ELx
    ldp     x10, x11, [sp, #CTX_OFFS_SP]
    msr     SPSR,   x11

    ldp     x0,  x1,  [sp, #16 * 0]
    ldp     x2,  x3,  [sp, #16 * 1]
    ldp     x4,  x5,  [sp, #16 * 2]
    ldp     x6,  x7,  [sp, #16 * 3]
    ldp     x8,  x9,  [sp, #16 * 4]
    ldp     x10, x11, [sp, #16 * 5]
    ldp     x12, x13, [sp, #16 * 6]
    ldp     x14, x15, [sp, #16 * 7]
    ldp     x16, x17, [sp, #16 * 8]
    ldp     x18, x19, [sp, #16 * 9]
    ldp     x20, x21, [sp, #16 * 10]
    ldp     x22, x23, [sp, #16 * 11]
    ldp     x24, x25, [sp, #16 * 12]
    ldp     x26, x27, [sp, #16 * 13]
    ldp     x28, x29, [sp, #16 * 14]
    ldr     x30, [sp, #CTX_OFFS_LR]

    add     sp, sp, #CTX_SIZE

    // return from exception
    eret
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
    ventry  cur_el_fiq                     // FIQ
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

    //set_irq_stack

    bl      decode_irq
    //mov x0, sp
    //bl      __test

    restore_context

    b .

cur_el_fiq:
    store_context

    //set_irq_stack

    b       decode_fiq
    b .

cur_el_serr:
    bl      cel_serr_traps
    eret
    b   .

lower_el_sync:
    store_context

    //set_irq_stack

    bl      lel_sync_traps
    eret
    b .

lower_el_irq:
    store_context

    //set_irq_stack

    b       decode_irq
    b .

lower_el_serr:
    bl      lel_serr_traps
    eret
    b .

.section ".stack", "wa"
irq_stack:
    .space CONFIG_DEFAULT_TASK_STACKSIZE
