#include <uk/param.h>
#include <machine/uk/asm.h>
	
#define MULTIBOOT_HDRSIGN 0x1badb002 
#define MULTIBOOT_REGSIGN 0x2badb002
#define _BOOT_FLAGS       0x00000003

#define MBIOFFSET_FLAGS   0
#define _MBIF_CMDLINE     (1 << 3)
#define MBIOFFSET_CMDLINE 16

ENTRY(start)
ENTRY(_start)
	jmp multiboot_entry

	.align 4
multiboot_header:
	.long MULTIBOOT_HDRSIGN
	.long _BOOT_FLAGS
	.long -(MULTIBOOT_HDRSIGN+_BOOT_FLAGS)
	.long multiboot_header
	.long _start
	.long _edata
	.long _end
	.long multiboot_entry

multiboot_entry:
	movl $(stacktop - UKERNBASE), %esp

	/* Clear flags (cld and cli) */
	pushl $0
	popf

	/* Check Multiboot */
	cmpl $MULTIBOOT_REGSIGN, %eax
	jne badmultiboot

	/* Setup boot paging structures */
	mov $0, %eax
	mov $(8 * 4), %ecx
	mov $UKERN_BL3TABLE, %edi
	rep stosb
	movl $(UKERN_BL2TABLE + 1), UKERN_BL3TABLE
	movl $(UKERN_BL2TABLE + 1), UKERN_BL3TABLE + 24
	mov $0, %eax
	mov $4096, %ecx
	mov $UKERN_BL2TABLE, %edi
	rep stosb

	mov  $0xe3, %eax
	mov  $512, %ecx
	mov  $UKERN_BL2TABLE, %edi
1:	mov  %eax, (%edi)
	add  $(1 << 21), %eax
	add  $8, %edi
	loop 1b

	/* Enable paging */
	mov %cr4, %eax
	or  $0x30, %eax
	mov  %eax, %cr4
	mov $UKERN_BL3TABLE, %eax
	mov %eax, %cr3
	mov  %cr0, %eax
	or   $(1<<31), %eax
	mov  %eax, %cr0
	ljmp $0x08, $1f
1:
	/* Set segments in a known state */
	call _C_LABEL(_setup_boot_gdt)
	lgdt UKERN_BGDTREG
	ljmp $0x8, $1f
1:	mov $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov $0, %eax
	mov %ax, %fs
	mov %ax, %gs

	/* Setup IDT */
	call _C_LABEL(_setup_idt)
	lidt _C_LABEL(_kern_idtr)

	/* Save Multiboot CMDLINE */
	mov MBIOFFSET_FLAGS(%ebx), %eax
	and $_MBIF_CMDLINE, %eax
	jz 1f
	mov MBIOFFSET_CMDLINE(%ebx), %eax
	jmp 2f
1:      xor %eax, %eax
2:	mov %eax, _C_LABEL(_boot_cmdline)

	/* Prepare real mode re-entry */
	mov $start_code16, %esi
	mov $UKERN_BCODE16, %edi
	mov $size_code16, %ecx
1:      rep movsb

	/* Call Real mode */
	mov $UKERN_BCODE16, %eax
	call *%eax

	/* Rebase pointers still in use */
	addl $UKERNBASE, %esp
	addl $UKERNBASE, vptr
	addl $UKERNBASE, vptrmax
	addl $UKERNBASE, vptrscrsrc
	addl $UKERNBASE, vptrscrdst

	call sysboot
halt:
	pushl $haltstring
	call printf
	add $4, %esp
loop: 
	cli
	hlt
	jmp loop

badmultiboot:
	lea badbootstring, %esi
	call print
	jmp loop


print:
	movl $0x1000, %ecx
	movb $0x87, %ah
	movl $0xb8000, %edi
1:	lodsb
	stosw
	cmp $0, %al
	loopne 1b
	ret

ENTRY(_setup_boot_gdt)
	/* Zero whole page */
	mov $0, %eax
	mov $1024, %ecx
	mov $UKERN_BGDTREG, %edi
	rep stosl

	mov $UKERN_BGDTREG, %eax
	movw $(128 * 8 - 1), (%eax)
	movl $(UKERNBASE + UKERN_BGDTABLE), 2(%eax)

	movl $UKERN_BGDTABLE + 8, %eax
	/* 32-bit code */
	movl $0x0000ffff, (%eax)
	movl $0x00cf9a00, 4(%eax)
	add $8, %eax
	/* 32-bit data */
	movl $0x0000ffff, (%eax)
	movl $0x00cf9200, 4(%eax)
	add $8, %eax
	/* 16-bit code */
	movl $0x0000ffff, (%eax)
	movl $0x00009a01, 4(%eax)
	add $8, %eax
	/* 16-bit code */
	movl $0x0000ffff, (%eax)
	movl $0x00009201, 4(%eax)
	add $8, %eax
	ret

	.globl _C_LABEL(_load_tss)
_C_LABEL(_load_tss):
	push  %ebp
	mov   %esp, %ebp
	movl  8(%ebp), %eax
	shl   $3, %eax
	add $(UKERNBASE + UKERN_BGDTABLE + (5 * 8)), %eax
	movl $0, (%eax)
	movl $0, 4(%eax)
	movl $(UKERNBASE +_C_LABEL(_bsp_tss)), %ecx
	movw %bx, 2(%eax)
	movw $104, (%eax)
	shr  $16, %ecx
	movb %cl, 4(%eax)
	movb %ch, 7(%eax)
	orl  $0x00408900, 4(%eax)
	mov  8(%ebp), %eax
	add  $5, %eax
	shl  $3, %eax
	ltr  %ax
	pop  %ebp
	ret

	.data
haltstring: 
	.asciz "System Halted."
haltstring_size = . - haltstring
	
badbootstring: 
	.asciz "Multiboot compliant bootloader needed."
badbootstring_size = . - badbootstring

LABEL(_ukern_end)
	.long _end

	.align 4096
stackbtm:
	.skip 4096
stacktop:
	.skip 512
LABEL(_bsp_tss)
	.skip 102
	.hword 104 // IOMAP over the edge.

