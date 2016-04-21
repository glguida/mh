# boot must be first
TMP= boot.S $(SRCS)
SRCS:= $(TMP)
SRCS+= biosops.S idt.S text.S locks.S pae.S
SRCS+= cpu.c apic.c sysboot.c tlb.c pci.c

# Used by MI part.
SRCS+= pmap.c machdep.c

#Used to embed init
SRCS+= empty.S

VPATH+= i386/src
