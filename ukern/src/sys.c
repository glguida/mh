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


#include <machine/uk/cpu.h>
#include <uk/sys.h>
#include <uk/kern.h>
#include <lib/lib.h>

static int sys_putc(int ch)
{
	sysputc(ch);
	return 0;
}

static int sys_die(void)
{
	die();
	return 0;
}

static int sys_xcptentry(vaddr_t entry, vaddr_t frame, vaddr_t stack)
{
	struct thread *th = current_thread();

	if (!__usraddr(entry)
            || !__usraddr(frame)
            || !__usraddr(frame + sizeof(struct xcptframe))
            || !__usraddr(stack))
		return -1;

	th->xcptframe = (void *) frame;
	__xcptframe_setup(&th->xcptentry, entry, stack);
	th->flags |= THFL_XCPTENTRY;
	return 0;
}

static void sys_xcptreturn(unsigned long rc)
{
	int ret;
	struct thread *th = current_thread();

	if (!(th->flags & THFL_IN_XCPTENTRY)
	    || !(th->flags & THFL_XCPTENTRY))
		goto _err;
	if (rc)
		goto _err;

	if (__xcptframe_usrupdate(&th->usrentry, th->xcptframe))
		goto _err;

	th->flags &= ~THFL_IN_XCPTENTRY;
	th->flags |= THFL_IN_USRENTRY;
	__insn_barrier();
	__xcptframe_enter(&th->usrentry);
	/* Not reached */
      _err:
	die();
	return;
}

int sys_call(int sc, unsigned long a1, unsigned long a2, unsigned long a3)
{
	switch (sc) {
	case SYS_PUTC:
		return sys_putc(a1);
	case SYS_DIE:
		return sys_die();
	case SYS_XCPTENTRY:
		return sys_xcptentry(a1, a2, a3);
	case SYS_XCPTRET:
		sys_xcptreturn(a1);
		return -1;
	default:
		return -1;
	}
}
