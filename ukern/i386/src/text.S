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


#include <machine/uk/asm.h>

WEAK_ALIAS(_boot_putc, b800_putc)
WEAK_ALIAS(_boot_sysputc, b800_sysputc)

	.data
ENTRY(base)
	.long 0

ENTRY(vptrscrsrc)
	.long 0xb80a0
END(vptrscrsrc)

ENTRY(vptrscrdst)
	.long 0xb8000
END(vptrscrdst)

ENTRY(vptr)
	.long 0xb8000 + 80 * 24 + 2
END(vptr)

ENTRY(vptrmax)
	.long 0xb8000 + 80 * 25 * 2
END(vptrmax)

/* HACK AHEAD */

ENTRY(b800_sysputc)
	mov $6, %ah
	jmp .LPRINT

ENTRY(b800_putc)
	mov $4, %ah
	jmp .LPRINT

.LPRINT:
	push  %ebp
	mov   %esp, %ebp
	movb  8(%ebp), %al
	movl  vptr, %ecx
	cmp   $32, %al
	jb    1f
	
	/* print */
	movw  %ax, (%ecx)
	add   $2, %ecx
	jmp   .Lupdate
	
1:	/* special chars */
	cmp   $9, %al
	jne   1f
	/* tab: 8 chars */
	add   $16, %ecx
	and   $-16, %ecx
	jmp   .Lupdate
	
1:	cmp   $10, %al
	jne   1f
	/* lf */
	xor   %eax, %eax
	xor   %edx, %edx
	mov   $160, %dl
	mov   %cx, %ax
	and   $0xfff, %ax
	div   %dl
	sub   %ah, %dl
	add   %edx, %ecx
	jmp   .Lupdate
	
.Lupdate:
	cmpl  vptrmax, %ecx
	jge   2f
1:
	xor   %eax, %eax
	mov   %ecx, vptr 
	pop   %ebp
	ret
2:	/* scroll */
	mov   %ecx, %eax
	pushl %esi
	pushl %edi
	movl  vptrscrsrc, %esi
	movl  vptrscrdst, %edi
	movl  $0x7b0, %ecx
	rep   movsl
	sub   $160, %eax
	popl  %edi
	popl  %esi
	mov   %eax, %ecx
	jmp .Lupdate
END(b800_putc)

