#include <machine/uk/asm.h>
#include "conf.h"

LABEL(start_code16)
#define ACTADDR(_x) ADDR_CODE16 + _x - _C_LABEL(start_code16)
#define RELADDR(_x) _x - _C_LABEL(start_code16)
	.code32
	mov %esp, _stackptr
	sgdt _saved_idtr

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
	mov %eax, %cr0

	ljmpl $SEG16_ADDR(ADDR_CODE16), $RELADDR(1f)
1:
	cs lidt RELADDR(_bios_idtr)

	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $SEG16_ADDR(ADDR_STCK16), %ax
	mov %ax, %ss
	mov $OFF16_ADDR(ADDR_STCK16), %sp

	/* Get E820 map */
	mov $SEG16_ADDR(ADDR_SMAP16), %ax
	mov %ax, %es
	mov $OFF16_ADDR(ADDR_SMAP16), %di
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
	orl $1, %eax
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

	mov _stackptr, %esp
	ret


_stackptr:
	.long 0	

	.align 64
_bios_idtr:
	.word 1023
	.long 0

	.align 64
_saved_idtr:
	.skip 6
	
	.globl _C_LABEL(size_code16)
size_code16 = . - start_code16
END(start_code16)

