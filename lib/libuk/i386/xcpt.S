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


#include <machine/asm.h>

ENTRY(__inthdlr)
	push %ebp
	mov %esp, %ebp

	pushfl
	push %eax
	push %ecx
	push %edx
	push %ebx
	push 16(%ebp) /* arg3: esp */
	push %esi
	push %edi

	pushw %gs
	pushw %fs
	pushw %es
	pushw %ds

	mov 4(%ebp), %eax /* ret addr: ip */
	push %eax

	push %esp /* frame -> &frame */
	mov 12(%ebp), %eax /*arg2: cause */
	push %eax
	mov 8(%ebp), %eax /* arg1: vect */
	push %eax
	call _C_LABEL(__sys_inthandler)
	add $12, %esp /* skip arguments */

	popl __per_thread_libuk_eip

	popw %gs
	popw %fs
	popw %es
	popw %ds

	pop %edi
	pop %esi
	popl __per_thread_libuk_esp
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax
	popfl
	pop %ebp
	mov __per_thread_libuk_esp, %esp
	jmpl *__per_thread_libuk_eip
END(__inthdlr)


	.data
	.align 64
LABEL(__per_thread_libuk_eip)
	.skip 8
END(__per_thread_libuk_eip)
LABEL(__per_thread_libuk_esp)
	.skip 8
END(__per_thread_libuk_esp)
