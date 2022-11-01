#ifndef __PL011_UART_H
#define __PL011_UART_H

/**************/
// QEMU-virt
#define MMIO_PBASE      (0x0)
#define MMIO_VBASE      (CONFIG_KERNEL_DEVICE_EXEC_ADDR)
#define MMIO_SIZE       (0x40000000)

#define GIC_OFFSET      (0x08000000)
#define GIC_PBASE       (MMIO_PBASE + GIC_OFFSET)
#define GIC_VBASE       (MMIO_VBASE + GIC_OFFSET)

#define UART_OFFSET     (0x09000000)
#define UART_PBASE      (MMIO_PBASE + UART_OFFSET)
#define UART_VBASE      (MMIO_VBASE + UART_OFFSET)

#define TIMER_CLOCK_HZ  (CONFIG_SYSTEM_CLOCK_HZ)
/**************/

#define UARTDR      0x000
#define UARTFR      0x018

#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)

#define MMIO_UARTDR     (UART_VBASE + UARTDR)
#define MMIO_UARTFR     (UART_VBASE + UARTFR)

typedef struct uart_dev {
    char name[8];
    unsigned long base;
} uart_dev_t;

void uart_init(unsigned long base);
void uart_putchar(char c);
char uart_getchar(void);

#endif
