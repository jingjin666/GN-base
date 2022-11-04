#include <kernel.h>
#include <k_stdint.h>
#include <uapi/util.h>

#include <uart.h>

#define UARTDR      0x000
#define UARTFR      0x018

#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)

static uart_dev_t pl011_dev;

void uart_init(unsigned long base)
{
    pl011_dev.base = base;
}

void uart_putchar(char c)
{
    if (c == '\n') {
        uart_putchar('\r');
    }

    while ((getreg64(pl011_dev.base + UARTFR) & UARTFR_TXFF) != 0);
    putreg64(c, pl011_dev.base + UARTDR);
}

char uart_getchar(void)
{
    while ((getreg64(pl011_dev.base + UARTFR) & UARTFR_RXFE) != 0);
    return getreg64(pl011_dev.base + UARTDR) & 0xff;
}

