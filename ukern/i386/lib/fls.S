#include <machine/uk/asm.h>

ENTRY(fls)
	push %ebp
	mov %esp, %ebp

	mov 8(%ebp), %eax
	cmp $0, %eax
	jz 1f

	bsr %eax, %eax
	inc %eax
1:
	mov %ebp, %esp
	pop %ebp
	ret
