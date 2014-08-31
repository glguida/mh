#include <machine/uk/asm.h>

ENTRY(memset)
	push %ebp
	mov  %esp, %ebp
	push %edi
	push %ebx

	mov  8(%ebp), %edi
	mov 12(%ebp), %eax
	mov 16(%ebp), %ecx

	mov %esi, %ebx
1:	rep stosb
	mov %ebx, %eax

	pop %ebx
	pop %edi
	mov %ebp, %esp
	pop %ebp

