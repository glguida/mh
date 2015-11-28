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
#include <uk/sys.h>

ENTRY(__inthdlr)
	push %ebp
	mov %esp, %ebp

	pushfl
	push %eax
	push %ecx
	push %edx
	push %ebx
	push %esi
	push %edi

	pushw %gs
	pushw %fs
	pushw %es
	pushw %ds

	push %esp /* frame -> &frame */
	pushl 12(%ebp) /* arg3: err */
	pushl 8(%ebp)  /* arg2: va */
	pushl 4(%ebp)  /* arg1: vect */
	call _C_LABEL(_libuk_signal_handler)
	add $16, %esp /* skip arguments */

	popw %ds
	popw %es
	popw %fs
	popw %gs

	pop %edi
	pop %esi
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax

	popfl
	pop %ebp

	add $12, %esp /* Skip info + xcpt */

	push %eax
	mov $SYS_IRET, %eax
	int $0x80
END(__inthdlr)
