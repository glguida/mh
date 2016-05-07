SRCS+= fls.S _setjmp.S
VPATH+= i386/lib

# compiler_rt 
SRCS+= divdi3.S moddi3.S udivdi3.S umoddi3.S _setjmp.S
VPATH+= $(SRCROOT)/exts/compiler_rt/lib/builtins/i386/