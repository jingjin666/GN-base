#ifndef __K_STDLIB_H
#define __K_STDLIB_H

#include <chinos/config.h>

void *k_calloc(size_t, size_t);
void k_free(void *);

/* Random number generation */
void      srand(unsigned int seed);
int       rand(void);
#define   srandom(s) srand(s)
long      random(void);
#endif
