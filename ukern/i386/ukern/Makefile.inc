SRCS+= boot.S biosops.S idt.S text.S locks.S pae.S
SRCS+= cpu.c lapic.c sysboot.c intrs.c pmap.c tlb.c __usrentry.c

#Used to embed init
SRCS+= empty.S

VPATH+= i386/ukern
