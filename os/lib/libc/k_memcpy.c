#include <k_stddef.h>
#include <k_string.h>

#include <chinos/config.h>

void *k_memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *pout = (unsigned char *)dest;
    unsigned char *pin  = (unsigned char *)src;
    while (n-- > 0) *pout++ = *pin++;
    return dest;
}

int k_memcmp(const void *vl, const void *vr, size_t n)
{
	const unsigned char *l=vl, *r=vr;
	for (; n && *l == *r; n--, l++, r++);
	return n ? *l-*r : 0;
}

