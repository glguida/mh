VPATH+=arch/$(MACHINE)/stdlib stdlib

SRCS+= _env.c abort.c atexit.c exit.c getenv.c simple_malloc.c

include arch/$(MACHINE)/stdlib/Makefile.inc
