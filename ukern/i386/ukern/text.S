#include <machine/uk/asm.h>

WEAK_ALIAS(_boot_putc, b800_putc)

	.data
vptr:
	.long 0xb8000
END(vptr)
	
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
	cmpl  $0xb9000, %ecx
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
	movl  $0xb80a0, %esi
	movl  $0xb8000, %edi
	movl  $0x7b0, %ecx
	rep   movsl
	sub   $160, %eax
	popl  %edi
	popl  %esi
	mov   %eax, %ecx
	jmp .Lupdate
END(b800_putc)

