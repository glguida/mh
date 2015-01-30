SRCS+= memcpy.S memset.S fls.S
VPATH+= i386/lib

# compiler_rt 
SRCS+= divdi3.S moddi3.S udivdi3.S umoddi3.S _setjmp.S
VPATH+= bsd/compiler_rt/lib/builtins/i386/