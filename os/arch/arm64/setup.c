#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <uart.h>

void BOOTPHYSIC boot_setup_mmu(void)
{
    uart_init(UART_PBASE);

    kprintf("hallo world\n");
    while(1);
}

