#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define wfi()       asm volatile("wfi" : : : "memory")

int data = 0x1234;
int bss;
int main(int argc, char *argv[])
{
    printf("\n\n---hello world---\n\n");

    printf("argc = %p, argv = %p\n", argc, argv);

    printf("data = %p, bss = %p\n", data, bss);

    char test[10];
    memset(test, 0, 10);

    //void *p = malloc(1028);
    //printf("malloc p = %p\n", p);
    //free(p);
    while(1)
    {
        printf("main data = %p, bss = %p\n", data++, bss--);
        wfi();
    }
}
