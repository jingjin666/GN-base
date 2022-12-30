#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <ipc.h>

extern char vm_start[];
extern char vm_end[];

#define MRS(reg, v) asm volatile("mrs %x0," reg : "=r"(v))
#define MSR(reg, v)                                \
    do {                                           \
        unsigned long _v = v;                             \
        asm volatile("msr " reg ",%x0" :: "r" (_v));\
    }while(0)

int data = 0x1234;
int bss;
int main(int argc, char *argv[])
{
    printf("\n\n---hello world---\n\n");

    printf("argc = %p, argv = %p\n", argc, argv);

    printf("data = %p, bss = %p\n", data, bss);

    char test[10];
    memset(test, 0, 10);

#if 0
    void *addr;
    void *addr1;
    void *addr2;
    void *addr3;
    void *addr4;
    
    addr  = malloc(16);
    printf("malloc: addr = %p\n", addr);

    addr1 = malloc(32);
    printf("malloc: addr1 = %p\n", addr1);

    addr2 = malloc(16);
    printf("malloc: addr2 = %p\n", addr2);

    addr3 = malloc(32);
    printf("malloc: addr3 = %p\n", addr3);

    free(addr1);
    free(addr2);

    addr4 = malloc(127*1024);
    printf("malloc: addr4 = %p\n", addr4);

    addr4 = malloc(1*1024);
    printf("malloc: addr4 = %p\n", addr4);

    addr4 = malloc(128*1024);
    printf("malloc: addr4 = %p\n", addr4);
    free(addr4);
#endif

    printf("vm_start = %p, vm_end = %p\n", vm_start, vm_end);

    printf("%p\n", *(unsigned long *)vm_start);


    thread_create(vm_start, 0);
    while(1)
    {
        //printf("main data = %p, bss = %p\n", data++, bss--);
        sleep(1);
    }
}

