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

#include <sys/inttypes.h>
#include <machine/vmparam.h>
#include <microkernel.h>
#include <drex/drex.h>
#include <drex/lwt.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <string.h>
#include <assert.h>

static void framedump(struct intframe *f)
{

	printf("EIP: %x\tEFLAGS: %x\n", f->eip, f->eflags);
	printf("\tEAX: %x\tEBX: %x\tECX: %x\tEDX:%x\n",
	       f->eax, f->ebx, f->ecx, f->edx);
	printf("\tEDI: %x\tESI: %x\tEBP: %x\tESP:%x\n",
	       f->edi, f->esi, f->ebp, f->esp);
	printf("\tDS: %x\tES: %x\tFS: %x\tGS: %x\n",
	       f->ds, f->es, f->fs, f->gs);
}

int __sys_faulthandler(unsigned vect, u_long va,
		       u_long err, struct intframe *f)
{
	extern lwt_t *lwt_current;
	printf("\nException %d, va = %p, err = %lx\n", vect, va, err);

	if (lwt_current != NULL
	    && (membar_consumer(), lwt_current->flags & LWTF_XCPT)) {
	  framelongjmp(f, &lwt_current->xcptbuf);
	  return 0;
	}
	
	framedump(f);
	printf("\n");
	return 0;
}

int do_child()
{
	pid_t pid;
	struct sys_childstat cs;

	do {
		pid = sys_childstat(&cs);
		printf("-> %d: %d\n", pid, cs.exit_status);
	} while (pid > 0);
}

int main()
{
	pid_t pid;

	printf("Hello!\n");
	sys_cli();
	printf("!Ue!\n");

	if (sys_fork() == 0)
		goto child1;
	if (sys_fork() == 0)
		goto child;
	if (sys_fork() == 0)
		goto child;
	inthandler(INTR_CHILD, do_child);
	lwt_sleep();
 child1:
	if (sys_fork())
		sys_die(21);
	/* Child */
 child:
	printf("Goodbye!\n");
	sys_die(5);
#if 0
	int j;
	size_t len;
	struct diodevice *dc;
	struct iovec iov[11];
	uint64_t val;
	

		do {
			dc = dirtio_pipe_open(500);
		} while(dc == NULL);

		dirtio_mmio_inw(&dc->desc, PORT_DIRTIO_MAGIC, 0, &val);
		printf("PORT_DIRTIO_MAGIC is %"PRIx64"\n", val);
		dirtio_mmio_inw(&dc->desc, PORT_DIRTIO_MAGIC, 0, &val);
		printf("PORT_DIRTIO_MAGIC is %"PRIx64"\n", val);

		len = dirtio_allocv(dc, iov, 11, 11 * 128 * 1024 - 1);

		for (j = 0; j < 11; j++)
			printf("\t%p -> %d\n",
			       iov[j].iov_base, iov[j].iov_len);
		dioqueue_addv(&dc->dqueues[0], 1, 10, iov);
#endif

}
