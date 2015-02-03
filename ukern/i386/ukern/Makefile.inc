SRCS+= boot.S biosops.S idt.S text.S locks.S pae.S
SRCS+= cpu.c lapic.c sysboot.c intrs.c pmap.c tlb.c __usrentry.c
VPATH+= i386/ukern
