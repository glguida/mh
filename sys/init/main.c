/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/stdint.h>
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
