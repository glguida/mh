SRCS+= boot.S biosops.S idt.S text.S locks.S pae.S
SRCS+= cpu.c apic.c sysboot.c tlb.c

# Used by MI part.
SRCS+= pmap.c machdep.c

#Used to embed init
SRCS+= empty.S

VPATH+= i386/src
