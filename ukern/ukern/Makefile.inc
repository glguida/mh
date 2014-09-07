VPATH+= ukern
SRCS+= pfndb.c gfp.c fixmems.c

CUSTOBJS+= fixmem.o structs.o

fixmem.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_FIXMEM -o $@ $^

structs.o: slab.c
	  $(CC) -c $(CFLAGS) -D__SLAB_STRUCTS -o $@ $^
