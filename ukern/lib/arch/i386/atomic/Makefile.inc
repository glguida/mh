VPATH+= lib/arch/$(MACHINE)/atomic
VPATH+= lib/atomic

SRCS+=	atomic_add_64_cas.c atomic_add_64_nv_cas.c atomic_and_64_cas.c \
	atomic_and_64_nv_cas.c atomic_dec_64_cas.c atomic_dec_64_nv_cas.c \
	atomic_inc_64_cas.c atomic_inc_64_nv_cas.c atomic_or_64_cas.c \
	atomic_or_64_nv_cas.c atomic_swap_64_cas.c atomic.S	\
	atomic_nand_64_cas.c atomic_xor_64_cas.c atomic_sub_64_cas.c \
	atomic_cas_64_cas.c

SRCS+=	atomic_init_cas.c