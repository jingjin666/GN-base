#ifndef __PL011_UART_H
#define __PL011_UART_H

typedef struct uart_dev {
    char name[8];
    unsigned long base;
} uart_dev_t;

void uart_init(unsigned long base);
void uart_putchar(char c);
char uart_getchar(void);

#endif
