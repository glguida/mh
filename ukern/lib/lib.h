#ifndef __libukern_h
#define __libukern_h

#include <uk/cdefs.h>
#include <uk/stdarg.h>

#define memcpy __builtin_memcpy
#define memset __builtin_memset

int vprintf(const char *fmt, va_list ap);
int printf(const char *, ...) __printflike(1, 2);
void _setputcfn(int(*)(int));

#endif
