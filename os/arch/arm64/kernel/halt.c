#include <k_stdio.h>

#include <halt.h>

void halt(void)
{
    kprintf("kernel oops!!!\n");
    while(1);
}
