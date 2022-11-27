#include <k_stdio.h>
#include <instructionset.h>

#include <idle.h>

void idle(void)
{
    //int i = 0;
    while(1)
    {
        //kprintf("idle %d\n", i++);
        wfi();
    }
}
