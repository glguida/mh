#include <microkernel.h>

void putc(int c)
{
	sys_putc('a');
}

int main()
{
	int i;

	for (i = 0; i < 100; i++)
		putc('a');
}
