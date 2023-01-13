#ifndef __BOARD_H
#define __BOARD_H

#include <chinos/config.h>

// QEMU-virt
#define RAM_PBASE       (CONFIG_KERNEL_LOAD_ADDR)
#define RAM_VBASE       (CONFIG_KERNEL_EXEC_ADDR)
#define RAM_SIZE        (2 * 1024 * 1024 * 1024UL)  // 1G

#define MMIO_PBASE      (0x0)
#define MMIO_VBASE      (CONFIG_KERNEL_DEVICE_EXEC_ADDR)
#define MMIO_SIZE       (0x40000000)    // 1G

#define GIC_OFFSET      (0x08000000)
#define GIC_PBASE       (MMIO_PBASE + GIC_OFFSET)
#define GIC_VBASE       (MMIO_VBASE + GIC_OFFSET)

#define UART_OFFSET     (0x09000000)
#define UART_PBASE      (MMIO_PBASE + UART_OFFSET)
#define UART_VBASE      (MMIO_VBASE + UART_OFFSET)

#define TIMER_CLOCK_HZ  (CONFIG_SYSTEM_CLOCK_HZ)
#endif
