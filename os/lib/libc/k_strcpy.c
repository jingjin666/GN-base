#include <chinos/config.h>

#include <k_string.h>

char *k_strcpy(char *dest, const char *src)
{
    char *tmp = dest;
    while ((*dest++ = *src++) != '\0');
    return tmp;
}
