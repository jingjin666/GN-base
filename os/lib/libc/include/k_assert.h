#ifndef __K_ASSERT_H
#define __K_ASSERT_H

#include <chinos/config.h>

#include <k_stdio.h>
#include <halt.h>

#ifdef CONFIG_DEBUG_ASSERTIONS
#define assert(x) do { \
                        if(!(x)) \
                        {   \
						    kprintf("\033[31m"); \
						    kprintf(#x " failed at %s.%d\n", __FILE__, __LINE__); \
						    kprintf("\033[32m"); \
                            halt(); \
                        }\
                  } while(0)

#define PANIC() assert(0)
#else
#define assert(x)
#define PANIC()
#endif

#endif
