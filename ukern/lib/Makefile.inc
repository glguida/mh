VPATH+= lib/
SRCS+= printf.c memcmp.c

# Remove warnings
CUSTOBJS+= $(OBJDIR)/rb.o

$(OBJDIR)/rb.o: rb.c
		$(CC) -c $(CFLAGS) -Wno-unused -Wno-unused-but-set -o $@ $^
