#include <chinos/config.h>

#include <kernel.h>
#include <k_stdio.h>
#include <uapi/util.h>
#include <uart.h>
#include <board.h>
#include <irq.h>
#include <generic_timer.h>

#include "init.h"

void BOOTONLY init_kernel(void)
{
    // 串口初始化
    uart_init(UART_VBASE);

    // 现在开始使用串口
    kprintf("kernel init...\n");

    // cpu初始化
    cpu_init();

    // 中断初始化
    kprintf("irq_initialize\n");
    irq_initialize();

    // timer初始化
    kprintf("timer_init\n");
    timer_init();

    // 打开中断
    arch_local_irq_enable();
}
