#include <machine/uk/asm.h>
#include "conf.h"	

	.altmacro
.macro _do_entry vct
	pushal
	mov  %cr3, %eax
	push %eax
	mov %cr2, %eax
	push %eax
	pushw %gs
	pushw %fs
	pushw %es
	pushw %ds
	mov $KDS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	
	mov %esp, %eax
	pushl %eax
	pushl $\vct
	.if \vct < 32
	call _C_LABEL(xcpt_entry)
	.else
	call _C_LABEL(intr_entry)
	.endif
	add $8, %esp
.endm


.macro handler_e vct
	.globl hdlr_&\vct
	.align 64
hdlr_&\vct&:
	cli
	_do_entry \vct
	/* No real exit for now */
	hlt
.endm

.macro handler vct
	.globl hdlr_&\vct
	.align 64
hdlr_&\vct&:
	cli
	pushl $0
	_do_entry \vct
	/* No real exit for now */
	hlt
.endm
	

.macro repeat macro from to
	\macro \from
	.if \to-\from
	repeat \macro,%(\from+1),\to
	.endif
.endm

.macro setidt vct
	mov $hdlr_&\vct, %ecx
	mov $KCS, %ax
	shl $16, %eax
	mov %cx, %ax
	mov %eax, _C_LABEL(_kern_idt) + (\vct * 8)
	mov %ecx, %eax
	mov $0x8e00, %ax
	mov %eax, _C_LABEL(_kern_idt) + (\vct * 8) + 4
.endm

ENTRY(_setup_idt)
	repeat setidt 0, 31
	ret

	repeat handler 0, 7
	handler_e 8
	handler 9
	repeat handler_e 10, 14
	handler 15
	handler 16
	handler_e 17
	repeat handler 18, 31
	repeat handler 32, 63
	repeat handler 64, 127
	repeat handler 128, 191
	repeat handler 192, 255

.data
	.align 64
LABEL(_kern_idt)
	.skip  256 * 8

LABEL(_kern_idtr)
	.word (256 * 8) - 1
	.long _C_LABEL(_kern_idt)

