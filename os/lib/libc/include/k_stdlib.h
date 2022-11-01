#ifndef __K_STDLIB_H
#define __K_STDLIB_H

#include <chinos/config.h>

/* Random number generation */
void      srand(unsigned int seed);
int       rand(void);
#define   srandom(s) srand(s)
long      random(void);
#endif
