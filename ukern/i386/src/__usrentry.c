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


#include <uk/types.h>
#include <machine/uk/param.h>
#include <machine/uk/cpu.h>
#include <uk/kern.h>
#include <lib/lib.h>

void __xcptframe_enter(struct xcptframe *f)
{
	current_cpuinfo()->tss.esp0 =
		(uint32_t) current_thread()->stack_4k + 0xff0;
	current_cpuinfo()->tss.ss0 = KDS;
	___usrentry_enter((void *) f);
}

void __xcptframe_setxcpt(struct xcptframe *f, unsigned long xcpt)
{
	f->eax = xcpt;
}


int __xcptframe_usrupdate(struct xcptframe *f, struct xcptframe *usrf)
{
	int ret;

	ret = usercpy(&f->edi, &usrf->edi, 40 /* edi to eip */ );
	if (ret)
		return -1;

	ret = usercpy(&f->esp, &usrf->esp, 4);
	if (ret)
		return -1;

	return 0;
}

void __xcptframe_setup(struct xcptframe *f, vaddr_t ip, vaddr_t sp)
{
	memset(f, 0, sizeof(*f));
	f->ds = UDS;
	f->es = UDS;
	f->fs = UDS;
	f->gs = UDS;
	f->eip = ip;
	f->cs = UCS;
	f->eflags = 0x202;
	f->esp = sp;
	f->ss = UDS;
}

void __usrentry_save(struct xcptframe *f, void *ptr)
{
	memcpy(f, ptr, sizeof(*f));
}
