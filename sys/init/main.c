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
#include <sys/dirtio.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <string.h>
#include <syslib.h>
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

	printf("\nException %d, va = %p, err = %lx\n", vect, va, err);

	framedump(f);
	printf("\n");
	return 0;
}

#define DEV_QUEUES 2
unsigned qmax[DEV_QUEUES];
unsigned qsize[DEV_QUEUES];
unsigned qready[DEV_QUEUES];

lwt_t *lwt2;

void testlwt(void *arg)
{
	uintptr_t n = (uintptr_t)arg;
	unsigned long x,y;

	printf("LWT2: arg is %d\n", n);
	asm volatile("mov %%esp, %0\n" : "=r"(x));
	asm volatile("mov %%ebp, %0\n" : "=r"(y));
	printf("sp is %lx, bp is %lx\n", x, y);
	lwt_yield();
	printf("LWT2: Were we saying?\n");
	lwt_exit();
}

int main()
{
	struct sys_creat_cfg cfg;

	siginit();

	printf("Hello!\n");
	printf("brk: %x\n", drex_sbrk(50L * PAGE_SIZE));

	cfg.nameid = 500;
	cfg.vendorid = 0xf00ffa;
	cfg.deviceid = 1;
	if (sys_fork()) {
		printf("Parent!\n");
		dirtio_dev_init(DEV_QUEUES, qmax, qsize, qready);
		dirtio_dev_creat(&cfg, 0111);
		lwt_sleep();
		printf("Hah?\n");
	} else {
		uint64_t val;
		int desc;
		struct dirtio_desc dio;
		struct dirtio_hdr *hdr;


		hdr = drex_mmap(NULL, sizeof(struct dirtio_hdr),
				PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
		memset(hdr, 0, sizeof(*hdr));

		desc = dirtio_desc_open(500, (void *)hdr, &dio);
		printf("child! %d: %"PRIx64":%"PRIx64"\n", desc, dio.vendorid, dio.deviceid);

		dirtio_mmio_inw(&dio, PORT_DIRTIO_MAGIC, 0, &val);
		printf("PORT_DIRTIO_MAGIC is %"PRIx64"\n", val);
		dirtio_mmio_inw(&dio, PORT_DIRTIO_MAGIC, 0, &val);
		printf("PORT_DIRTIO_MAGIC is %"PRIx64"\n", val);

		lwt2 = lwt_create(testlwt, (void *)1, 1024);
		printf("Switching soon from lwt1\n");
		lwt_wake(lwt2);
		lwt_yield();
		printf("Hello again from lwt1!\n");
		lwt_wake(lwt2);
		lwt_yield();
		printf("Hm!\n");
		lwt_wake(lwt2);
		printf("Going\n");
		lwt_yield();

		{
			int j;
			size_t len;
			struct diodevice *dc;
			struct iovec iov[11];
			dc = dirtio_pipe_open(500);
			assert(dc);
			len = dirtio_allocv(dc, iov, 11, 11 * 128 * 1024);
			printf("len = %d\n", len);
			for (j = 0; j < 11; j++)
				printf("\t%p -> %d\n", iov[j].iov_base, iov[j].iov_len);

		}
		
		
	}

	printf("Goodbye!\n");
	sys_die();
}
