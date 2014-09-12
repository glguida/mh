VPATH+= ukern
SRCS+= pfndb.c pgalloc.c fixmems.c heap.c

CUSTOBJS+= $(OBJDIR)/fixmem.o $(OBJDIR)/structs.o

$(OBJDIR)/fixmem.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_FIXMEM -o $@ $^

$(OBJDIR)/structs.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_STRUCTS -o $@ $^
