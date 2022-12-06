#ifndef __K_STRING_H
#define __K_STRING_H

#include <chinos/config.h>

#include <k_types.h>

void *k_memset(void *s, int c, size_t n);
void *k_memcpy(void *dest, const void *src, size_t n);
int   k_memcmp(const void *vl, const void *vr, size_t n);

size_t k_strlen(const char *);
size_t k_strnlen(const char *, size_t);

char *k_strcpy(char *dest, const char *src);
char *k_strncpy(char *, const char *, size_t);

int k_strcmp(const char *, const char *);
#endif
