#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "root.h"

#define getreg64(a)           (*(volatile uint64_t *)(a))
#define putreg64(v,a)         (*(volatile uint64_t *)(a) = (v))

#define UARTDR      0x000
#define UARTFR      0x018

#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)

typedef struct user_uart_dev {
    char name[8];
    unsigned long base;
} user_uart_dev_t;

static struct user_uart_dev pl011_dev = {
    .base = 0x0000000f00000000ULL//CONFIG_USER_DEVICE_EXEC_ADDR
};

void uart_putchar(char c)
{
    if (c == '\n') {
        uart_putchar('\r');
    }

    while ((getreg64(pl011_dev.base + UARTFR) & UARTFR_TXFF) != 0);
    putreg64(c, pl011_dev.base + UARTDR);
}

void _printf(char *str)
{
    while (*str != '\0') {
        uart_putchar(*str++);
    }
}

int bss;
int main(int argc, char *argv[])
{
    char test[10];
    memset(test, 0, 10);
    bss++;
    _printf("hello chinos\n");

    return bss;
}
