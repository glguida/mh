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
#include <uk/param.h>

LABEL(start_code16)
#define RELADDR(_x) _x - _C_LABEL(start_code16)
	.code32
	mov %esp, _saved_esp
	sidt _saved_idtr

	cli
	mov $0x20, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss
	ljmpl $0x18, $RELADDR(1f)
1:
	.code16
	mov %cr0, %eax
	and $-2, %eax
	and $0x7fffffff, %eax
	mov %eax, %cr0
	ljmpl $SEG16_ADDR(UKERN_BCODE16), $RELADDR(1f)
1:
	cs lidt RELADDR(_bios_idtr)
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $SEG16_ADDR(UKERN_BSTCK16), %ax
	mov %ax, %ss
	mov $OFF16_ADDR(UKERN_BSTCK16), %sp

	/* Get E820 map */
	mov $SEG16_ADDR(UKERN_BSMAP16), %ax
	mov %ax, %es
	mov $OFF16_ADDR(UKERN_BSMAP16), %di
	mov $0xe820, %eax
	mov $0, %ebx
	mov $24, %ecx
	mov $0x534d4150, %edx	
1:	int $0x15
	jc 1f
	cmp $0, %ebx
	je 1f
	cmp $24, %ecx
	je 2f
	add $20, %di
	mov $0xff, %ax
	mov $2, %cx
	rep stosw
	mov $24, %ecx
	jmp 3f
2:	add $24, %di
3:	mov $0xe820, %eax
	mov $0x534d4150, %edx
	jmp 1b
	
	/* Set a zero entry */
1:	mov $12, %cx
	xor %ax, %ax
	rep stosw

	/* Get back to protected mode */
	cli
	mov %cr0, %eax
	orl $0x1, %eax
	mov %eax, %cr0
	orl $0x80000000, %eax
	mov %eax, %cr0
	ljmpl $0x18, $RELADDR(1f)
1:
	ljmpl $0x08, $1f
1:
	.code32
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	lidt _saved_idtr
	mov  _saved_esp, %esp
	ret

	.align 64
_bios_idtr:
	.word 1023
	.long 0

	.align 64
_saved_idtr:
	.skip 6
	
_saved_esp:
	.long 0
	
	.globl _C_LABEL(size_code16)
size_code16 = . - start_code16
END(start_code16)

