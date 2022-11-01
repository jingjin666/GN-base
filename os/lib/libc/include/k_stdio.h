#ifndef __K_STDIO_H
#define __K_STDIO_H

#include <chinos/config.h>

#include <k_stdarg.h>

struct va_format
{
    const char *fmt;
    va_list *va;
};

void kprintf(const char *fmt, ...);
void kputs(const char *s);
#endif
