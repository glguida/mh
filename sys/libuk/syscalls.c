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


#include <microkernel.h>
#include "__sys.h"

__dead void sys_die(void)
{
	int dummy;

	__syscall0(SYS_DIE, dummy);
	/* Not reached */
}

int sys_putc(int ch)
{
	int ret;

	__syscall1(SYS_PUTC, ch, ret);
	return ret;
}

int sys_inthdlr(void (*func) (vaddr_t, vaddr_t), void *stack)
{
	int ret;

	__syscall2(SYS_INTHDLR,
		   (unsigned long) func, (unsigned long) stack, ret);
	return ret;
}

void sys_sti(void)
{
	int dummy;

	__syscall0(SYS_STI, dummy);
}

void sys_cli(void)
{
	int dummy;

	__syscall0(SYS_CLI, dummy);
}

void sys_wait(void)
{
	int dummy;

	__syscall0(SYS_WAIT, dummy);
}

int sys_fork(void)
{
	int ret;

	__syscall0(SYS_FORK, ret);
	return ret;
}

int sys_map(vaddr_t vaddr, sys_map_flags_t perm)
{
	int ret;

	__syscall2(SYS_MAP,
		   (unsigned long) vaddr, (unsigned long) perm, ret);
	return ret;
}

int sys_move(vaddr_t dst, vaddr_t src)
{
	int ret;

	__syscall2(SYS_MOVE,
		   (unsigned long) dst, (unsigned long) src, ret);
	return ret;

}

int sys_open(u_long id)
{
	int ret;

	__syscall1(SYS_OPEN, (unsigned long) id, ret);
	return ret;
}

int sys_intmap(unsigned ddno, unsigned id, unsigned sig)
{
	int ret;

	__syscall3(SYS_INTMAP, (unsigned long) ddno, (unsigned long) id,
		   (unsigned long) sig, ret);
	return ret;

}

int sys_io(unsigned ddno, u_long port, u_long val)
{
	int ret;

	__syscall3(SYS_IO, (unsigned long) ddno, (unsigned long) port,
		   (unsigned long) val, ret);
	return ret;
}

int sys_creat(u_long id, unsigned sig)
{
	int ret;

	__syscall2(SYS_CREAT, (unsigned long) id, (unsigned long) sig,
		   ret);
	return ret;
}

int sys_poll(struct sys_poll_ior *ior)
{
	int ret;

	__syscall1(SYS_POLL, (unsigned long)ior, ret);
	return ret;
}

int sys_eio(unsigned id)
{
	int ret;

	__syscall1(SYS_EIO, (unsigned long)id, ret);
	return ret;
}