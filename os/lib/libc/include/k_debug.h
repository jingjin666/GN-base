#ifndef __K_DEBUG_H
#define __K_DEBUG_H

#include <chinos/config.h>

#include <k_stdio.h>

#define knone (void)

#ifdef CONFIG_DEBUG_FEATURES
#define kdbg(fmt, args ...) \
    do {kprintf("[DEBUG]%s:%d: " fmt, __FUNCTION__, __LINE__, ## args); } while(0)

#ifdef CONFIG_DEBUG_ERROR
#define kerr(fmt, args ...) \
    do {kprintf("[ERROR]%s:%d: " fmt, __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define kerr(fmt, ...)
#endif

#ifdef CONFIG_DEBUG_WARN
#define kwarn(fmt, args ...) \
    do {kprintf("[WARN]%s:%d: " fmt, __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define kwarn(fmt, ...)
#endif

#ifdef CONFIG_DEBUG_INFO
#define kinfo(fmt, args ...) \
    do {kprintf("[INFO]%s:%d: " fmt, __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define kinfo(fmt, ...)
#endif

#else
#define kdbg(fmt, ...)
#define kerr(fmt, ...)
#define kwarn(fmt, ...)
#define kinfo(fmt, ...)
#endif

#endif
