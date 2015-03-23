#include <microkernel.h>
#include <syslib.h>

void putc(int c)
{
	sys_putc(c);
}

int sys_sighandler(int sighandler, struct usrentry frame)
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


	printf("Hello!\n");
	for (i = 0; i < 100; i++)
		putc('a');
      asm("":::"memory");
	f();


	for (i = 0; i < 100; i++)
		putc('b');


}
