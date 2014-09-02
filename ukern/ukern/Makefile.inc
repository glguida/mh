VPATH+= ukern/
SRCS+= pfndb.c gfp.c

CUSTOBJS+= fixmem.o

fixmem.o: slab.c
	$(CC) -c $(CFLAGS) -D__SLAB_FIXMEM -o $(OBJDIR)/$@ $^
