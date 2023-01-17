#ifndef __K_STDLIB_H
#define __K_STDLIB_H

#include <chinos/config.h>

#include <k_types.h>

void *kmalloc(size_t);
void *kcalloc(size_t, size_t);
void kfree(void *);

/* Random number generation */
void      srand(unsigned int seed);
int       rand(void);
#define   srandom(s) srand(s)
long      random(void);
#endif
