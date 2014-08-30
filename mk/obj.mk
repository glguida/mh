OBJS= $(addsuffix .o, $(basename $(SRCS)))

%.o: %.c
	$(CC) -c $(CFLAGS) -o $(OBJDIR)/$@ $^

%.o: %.S
	$(CC) -c $(ASFLAGS) -o $(OBJDIR)/$@ $^
