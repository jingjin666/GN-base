#ifndef __K_STDIO_H
#define __K_STDIO_H

#include <chinos/config.h>

#include <k_types.h>
#include <k_stdarg.h>

struct va_format
{
    const char *fmt;
    va_list *va;
};

void kprintf(const char *fmt, ...);
void _kprintf(const char *fmt, ...);
void kputs(const char *s);

static inline int k_vsnprintf(char *buf, size_t size, const char *format, va_list ap)
{
    return 0;
}

#endif
