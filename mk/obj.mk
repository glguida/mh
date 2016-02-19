OBJS= $(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(SRCS))))

$(OBJDIR)/%.S.o: %.S
	$(CC) -c $(ASFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.S
	$(CC) -c $(ASFLAGS) -o $@ $^

$(OBJDIR)/%.c.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

ifneq ($(CUSTOBJS)z,z)
OBJS+= $(CUSTOBJS)
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

ALL_TARGET+=$(OBJDIR)
