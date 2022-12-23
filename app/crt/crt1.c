#include <stdint.h>

#define weak __attribute__((__weak__))
#define hidden __attribute__((__visibility__("hidden")))
#define weak_alias(old, new) extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

extern weak hidden void (*const __init_array_start)(void), (*const __init_array_end)(void);

int main(int argc, char *argv[]);

void _fini(void)
{
    // crtn.o
}

void _init(void)
{
    // crti.o
}

void libc_start_init(void)
{
	_init();

    // cxx contruction
	uintptr_t a = (uintptr_t)&__init_array_start;
	for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)()))
		(*(void (**)(void))a)();
}

void init_libc(void)
{
    // todo : tls
}

#if 0
#define getreg64(a)           (*(volatile uint64_t *)(a))
#define putreg64(v,a)         (*(volatile uint64_t *)(a) = (v))
#define UARTDR      0x000
#define UARTFR      0x018

#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)
int data = 0x1234;
int bss;
#endif

void _start(unsigned long *p)
{
#if 1
	int argc = p[0];
	char **argv = (void *)(p+1);

    init_libc();

    libc_start_init();

    int ret = main(argc, argv);

    // exit includes:
        // 1.atexit
        // 2.libc_exit_fini
        // 3.stdio_exit
    exit(ret);
#else
    while ((getreg64(0x0000000f00000000UL + UARTFR) & UARTFR_TXFF) != 0);
    putreg64('w', 0x0000000f00000000UL + UARTDR);
    int argc = p[0];
    argc++;
    while(1) {
        //asm volatile("isb sy" : : : "memory");
        data++;
        bss++;
        asm volatile("wfi" : : : "memory");
    }
#endif
}
