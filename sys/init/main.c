#include <microkernel.h>

void putc(int c)
{
	sys_putc(c);
}

int sys_sighandler(int sighandler, unsigned long a1, unsigned long a2)
{
	int i;

	for (i = 0; i < 20; i++)
		putc('E');

	return -1;
}

void (*f) (void) = NULL;

int main()
{
	int i;

	siginit();

	for (i = 0; i < 100; i++)
		putc('a');
	f();


	for (i = 0; i < 100; i++)
		putc('b');


}
