#include <machine/uk/asm.h>
#include "conf.h"
	
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
	movl $stacktop, %esp

	/* Clear flags (cld and cli) */
	pushl $0
	popf

	/* Check Multiboot */
	cmpl $MULTIBOOT_REGSIGN, %eax
	jne badmultiboot

	/* Set segments in a known state */
	lgdt _C_LABEL(_boot_gdtr)
	ljmp $0x8, $1f
1:	mov $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov $0, %eax
	mov %ax, %fs
	mov %ax, %gs

	/* Setup IDT */
	call _C_LABEL(_set_boot_idt)
	lidt _C_LABEL(_boot_idtr)
	
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
	mov $ADDR_CODE16, %edi
	mov $size_code16, %ecx
1:      rep movsb

	/* Call Real mode */
	mov $ADDR_CODE16, %eax
	call *%eax

	call sysboot
halt:
	lea haltstring, %esi
	call print
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

	.data
haltstring: 
	.asciz "System Halted."
haltstring_size = . - haltstring
	
badbootstring: 
	.asciz "Multiboot compliant bootloader needed."
badbootstring_size = . - badbootstring

LABEL(_boot_idtr)
	.word (256 * 8) - 1
	.long _C_LABEL(_boot_idt)
	
LABEL(_boot_gdtr)
	.word 5 * 8 - 1
	.long _C_LABEL(_boot_gdt)

    	.align 64
LABEL(_boot_gdt)
	/* Unused */
	.quad   0
	/* 32-bit code */
	.quad   0x00cf9a000000ffff
        /* 32-bit data */
        .quad   0x00cf92000000ffff
	/* 16-bit code */
	.quad   0x00009a000000ffff | (ADDR_CODE16 << 16)
	/* 16-bit data */
	.quad   0x000092000000ffff | (ADDR_CODE16 << 16)


/* BSP Bootstrap Page Structure: see AP Bootstrap Page Structure in ll.S */
	.align 4096
stackbtm:
	.skip (4096 - 104 - 4 - 4)
stacktop:
	.long 0    // CPU Number
	.long 0x50500505
LABEL(cpu_tss)
LABEL(bsptss) /* Per CPU? */
	.skip 102
	.hword 104 // IOMAP over the edge.

	.align 4
	.globl cpu_kstack
	.globl cpu_udata, cpu_inkern, cpu_idle, cpu_inustore
cpu_udata:
	.long 0
cpu_kstack:
	.long 0
cpu_idle:
	.long 0
cpu_inkern:
	.long 1
cpu_inustore:
	.long 0



