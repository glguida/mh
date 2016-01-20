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
#include <sys/dirtio.h>
#include <sys/mman.h>
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

int __sys_inthandler(int vect, u_long va, u_long err, struct intframe *f)
{
	static int i = 0;

	if (vect == INTR_EXT) {
		return;
	}
	printf("\nException %d, va = %p, err = %lx\n", vect, va, err);

	framedump(f);
	printf("\n");


}

#define DEV_QUEUES 2
unsigned qmax[DEV_QUEUES];
unsigned qsize[DEV_QUEUES];
unsigned qready[DEV_QUEUES];
struct dirtio_dev dev;


int main()
{
	int i;
	struct sys_creat_cfg cfg;

	siginit();


	printf("Hello!\n");

	printf("brk: %x\n", drex_sbrk(50L * PAGE_SIZE));

	cfg.nameid = 500;
	cfg.vendorid = 0xf00ffa;
	cfg.deviceid = 1;
	printf("%d creat()", sys_creat(&cfg, 9, 0111));

	if (sys_fork()) {
		int i;

		dirtio_dev_init(&dev, DEV_QUEUES, qmax, qsize, qready);
		printf("Parent!\n");
		while (1) {
			int ret;
			unsigned id;
			struct sys_poll_ior ior;

			sys_wait();
			sys_cli();
			id = sys_poll(&ior);
			printf("I/O port %x,  val %x\n", ior.port, ior.val);
			printf("Test: %d\n", dirtio_dev_io(&dev, id, ior.port, ior.val));
			sys_eio(id);
		}
	} else {
		int desc, ret;
		struct sys_creat_cfg cfg;
		int *p = drex_mmap(NULL, sizeof(struct dirtio_hdr),
				   PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);

		printf("child!\n");
		*p = 0;
		desc = sys_open(500);
		sys_mapirq(desc, 0, 5);
		printf("MAPPING %d\n", sys_export(desc, p, 0));
		sys_readcfg(0, &cfg);
		printf("cfg: %llx %lx %lx\n",
		       cfg.nameid, cfg.vendorid, cfg.deviceid);

		while (1) {
			sys_out(desc, PORT_DIRTIO_IN | (PORT_DIRTIO_MAGIC << 8), 5);
			sys_wait();
			printf("IRQ received! %x\n", *(uint32_t *)p);
			sys_out(desc, PORT_DIRTIO_IN | (PORT_DIRTIO_MAGIC << 8), 5);
			sys_wait();
			printf("IRQ received! %x\n", *(uint32_t *)p);
			while(1);
		}
	}

	printf("Goodbye!\n");
	sys_die();
}
