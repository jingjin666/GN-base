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

void _start(unsigned long *p)
{
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
}
