#include <chinos/config.h>

#include <k_string.h>

int k_strcmp(const char *cs, const char *ct)
{
    register signed char result;
    for (; ; )
    {
        if ((result = *cs - *ct++) != 0 || !*cs++)
            break;
    }

    return result;
}
