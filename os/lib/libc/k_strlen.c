#include <chinos/config.h>

#include <k_string.h>

size_t k_strlen(const char *s)
{
    const char *sc;
    for (sc = s; *sc != '\0'; ++sc);
    return sc - s;
}
