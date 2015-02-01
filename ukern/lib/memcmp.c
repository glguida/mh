#include <uk/types.h>
#include <lib/lib.h>

int
memcmp(void *s1, void *s2, size_t n)
{
    unsigned char *p1, *p2;

    if (!n)
	return 0;

    do {
	if (*p1++ != *p2++)
	    return (*--p1 - *--p2);
    } while (--n != 0);

    return 0;
}
