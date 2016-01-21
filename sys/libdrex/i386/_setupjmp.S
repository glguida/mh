#include <machine/asm.h>

ENTRY(_setupjmp)
	push %ebp
	mov %esp, %ebp
	mov 0x8(%ebp), %edx
	/* EIP */
	mov 0xc(%ebp), %ecx
	mov %ecx, 0(%edx)
	/* ESP */
	mov 0x10(%ebp), %ecx
	mov %ecx, 8(%edx)
	pop %ebp
	ret
END(_setupjmp)
