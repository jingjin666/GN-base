#include <k_stdio.h>
#include <instructionset.h>

#include <idle.h>

void idle(void)
{
    while(1)
    {
        wfi();
    }
}
