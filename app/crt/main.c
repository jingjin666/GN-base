#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <ipc.h>

extern char vm_start[];
extern char vm_end[];

#define wfi()       asm volatile("wfi" : : : "memory")

#define MRS(reg, v) asm volatile("mrs %x0," reg : "=r"(v))
#define MSR(reg, v)                                \
    do {                                           \
        unsigned long _v = v;                             \
        asm volatile("msr " reg ",%x0" :: "r" (_v));\
    }while(0)

#define getreg64(a)           (*(volatile uint64_t *)(a))
#define putreg64(v,a)         (*(volatile uint64_t *)(a) = (v))
#define UARTDR      0x000
#define UARTFR      0x018
#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)

int data = 0x1234;
int bss;

void hello_thread(void)
{
    int i = 0;
    printf("hello_thread\n");

    while(1)
    {
        printf("hello_thread %d\n", i++);
        sleep(1);
    }
}

unsigned long read_evt_cnt(void)
{
    unsigned long pmu_event_cnt;
    MSR("PMSELR_EL0", 0);
    MRS("PMXEVCNTR_EL0", pmu_event_cnt);
    return pmu_event_cnt;
}

void cache_misses_test(void)
{
    int i,c=0;
    for(i=0;i<100000;i++)
    {
        c+=i*i;
        c-=i*100;
        c+=i*i*i/100;
    }
    //printf("c = %d\n", c);
}

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

#if 0
    void *stack = malloc(8192);
    printf("stack = %p\n", stack);

    thread_create(hello_thread, stack);
#endif

//#define LOAD_VM
#ifdef LOAD_VM
    printf("vm_start = %p, vm_end = %p\n", vm_start, vm_end);

    printf("%p = %p\n", vm_start, *(unsigned long *)vm_start);

    unsigned long vm_base = (unsigned long)vm_start;
    unsigned long vm_size = vm_end - vm_start;
    vcpu_create((unsigned long)vm_start, 0, vm_base, vm_size);
#endif

    while(1)
    {
        //unsigned long pmu_ccnt_start, pmu_ccnt_end;
        //MRS("PMCCNTR_EL0", pmu_ccnt_start);
        
        //printf("+++\n");
        //printf("main data = %p, bss = %p\n", data++, bss--);
        //printf("---\n");

        cache_misses_test();

        unsigned long cnt = read_evt_cnt();
        //printf("%p\n", cnt);

        //sleep(1);
        //usleep(100000);
        
        //MRS("PMCCNTR_EL0", pmu_ccnt_end);

        //long diff = pmu_ccnt_end - pmu_ccnt_start;
        //printf("diff = %lu\n", diff);
    }
}

