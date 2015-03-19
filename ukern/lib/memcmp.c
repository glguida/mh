#include <uk/types.h>
#include <lib/lib.h>

int memcmp(void *s1, void *s2, size_t n)
{
	char *p1 = s1, *p2 = s2;

	if (!n)
		return 0;

	do {
		if (*p1++ != *p2++)
			return (*--p1 - *--p2);
	} while (--n != 0);

	return 0;
}
