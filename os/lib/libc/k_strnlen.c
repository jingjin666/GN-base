#include <chinos/config.h>

#include <k_string.h>

size_t k_strnlen(const char *s, size_t maxlen)
{
    const char *sc;
    for (sc = s; maxlen != 0 && *sc != '\0'; maxlen--, ++sc);
    return sc - s;
}
