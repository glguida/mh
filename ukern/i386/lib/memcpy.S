#include <machine/uk/asm.h>

ENTRY(memcpy)
	push %ebp
	mov %esp, %ebp
	push %edi
	push %esi

	mov 8(%ebp), %edi
	mov 12(%ebp), %esi
	mov 16(%ebp), %ecx
	rep movsb

	popl %esi
	popl %edi
	popl %ebp
	ret
