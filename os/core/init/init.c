#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <uapi/util.h>
#include <uart.h>

void BOOTONLY init_kernel(void)
{
    uart_init(UART_VBASE);
    kprintf("kernel init...\n");
}
