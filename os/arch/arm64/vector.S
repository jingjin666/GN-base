#include <chinos/config.h>
#include <registerset.h>

.global vector

.macro ventry label
.align 7
    b       \label
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
    bl      decode_irq
    eret
    b .

cur_el_serr:
    bl      cel_serr_traps
    eret
    b   .

lower_el_sync:
    bl      lel_sync_traps
    eret
    b .

lower_el_irq:
    b .

lower_el_serr:
    bl      lel_serr_traps
    eret
    b .
