#include <microkernel.h>
#include <syslib.h>

void putc(int c)
{
	sys_putc(c);
}

int test()
{
	printf("\tFixed up!\n");
}

int sys_sighandler(int sighandler, struct xcptframe frame)
{
	int i;
	static unsigned long esp, ebp;

	printf("Exception %d, va = %p\n", sighandler, frame.cr2);

	if (frame.cr2 == -1) {
		frame.eip = (uintptr_t) test;
		frame.ebp = ebp;
		frame.esp = esp;
	} else {
		char *p = &frame;
		int i;
		esp = frame.esp;
		ebp = frame.ebp;

		for (i = 0; i < sizeof(struct xcptframe); i++)
			*p++ = -1;
	}
	return 0;
}

void (*f) (void) = (void (*)(void)) 0x50500505;



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
