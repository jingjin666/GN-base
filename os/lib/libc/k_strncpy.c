#include <chinos/config.h>

#include <k_stddef.h>
#include <k_string.h>

char *k_strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;     /* Value to be returned */
    char *end = dest + n; /* End of dest buffer + 1 byte */

    /* Copy up n bytes, breaking out of the loop early if a NUL terminator is
    * encountered.
    */

    while ((dest != end) && (*dest++ = *src++) != '\0')
    {
    }

    /* Note that there may be no NUL terminator in 'dest' */

    /* Pad the remainder of the array pointer to 'dest' with NULs */

    while (dest != end)
    {
        *dest++ = '\0';
    }

    return ret;
}
