OBJS= $(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(SRCS))))

$(OBJDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.S
	$(CC) -c $(ASFLAGS) -o $@ $^

OBJS+= $(CUSTOBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

ALL_TARGET+=$(OBJDIR)
