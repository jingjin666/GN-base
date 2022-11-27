#include "stdio_impl.h"
#include <sys/uio.h>

#if 1
#define getreg(a)           (*(volatile unsigned long *)(a))
#define putreg(v,a)         (*(volatile unsigned long *)(a) = (v))

#define UARTDR      0x000
#define UARTFR      0x018

#define UARTFR_TXFF (1 << 5)
#define UARTFR_RXFE (1 << 4)

#define UART_UBASE   0x0000000f00000000UL//CONFIG_USER_DEVICE_EXEC_ADDR
static void uart_putchar(char c)
{
    if (c == '\n') {
        uart_putchar('\r');
    }

    while ((getreg(UART_UBASE + UARTFR) & UARTFR_TXFF) != 0);
    putreg(c, UART_UBASE + UARTDR);
}
#endif

size_t __stdio_write(FILE *f, const unsigned char *buf, size_t len)
{
    	struct iovec iovs[2] = {
    		{ .iov_base = f->wbase, .iov_len = f->wpos-f->wbase },
    		{ .iov_base = (void *)buf, .iov_len = len }
    	};         
    	struct iovec *iov = iovs;
    	size_t rem = iov[0].iov_len + iov[1].iov_len;
    	int iovcnt = 2;
    	ssize_t cnt;
#if 1
        char *str = (char *)iov[0].iov_base;
        while (iov[0].iov_len--) {
            uart_putchar(*str++);
        }

        str = (char *)iov[1].iov_base;
        while (iov[1].iov_len--) {
            uart_putchar(*str++);
        }

        f->wend = f->buf + f->buf_size;
        f->wpos = f->wbase = f->buf;

        return len;
#else   
    	for (;;) {
    		cnt = syscall(SYS_writev, f->fd, iov, iovcnt);
    		if (cnt == rem) {
    			f->wend = f->buf + f->buf_size;
    			f->wpos = f->wbase = f->buf;
    			return len;
    		}
    		if (cnt < 0) {
    			f->wpos = f->wbase = f->wend = 0;
    			f->flags |= F_ERR;
    			return iovcnt == 2 ? 0 : len-iov[0].iov_len;
    		}
    		rem -= cnt;
    		if (cnt > iov[0].iov_len) {
    			cnt -= iov[0].iov_len;
    			iov++; iovcnt--;
    		}
    		iov[0].iov_base = (char *)iov[0].iov_base + cnt;
    		iov[0].iov_len -= cnt;
    	}
    }
#endif
}
