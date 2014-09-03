#include <machine/uk/asm.h>

WEAK_ALIAS(_boot_putc, b800_putc)

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
	.long 0xb8000
END(vptr)

ENTRY(vptrmax)
	.long 0xb8000 + 80 * 25 * 2
END(vptrmax)
	
ENTRY(b800_putc)
	push  %ebp
	mov   %esp, %ebp
	movb  8(%ebp), %al
	movl  vptr, %ecx
	cmp   $32, %al
	jb    1f
	
	/* print */
	mov   $1, %ah
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

