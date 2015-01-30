VPATH+= ukern
SRCS+= addrspc.c kern.c pfndb.c pgalloc.c fixmems.c heap.c vmap.c thread.c

CUSTOBJS+= $(OBJDIR)/fixmem.o $(OBJDIR)/structs.o

$(OBJDIR)/fixmem.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_FIXMEM -o $@ $^

$(OBJDIR)/structs.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_STRUCTS -o $@ $^
