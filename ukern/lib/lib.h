#ifndef __libukern_h
#define __libukern_h

#include <uk/cdefs.h>
#include <uk/stdarg.h>

int (*putc)(int);
void *memset(void *b, int c, size_t len);
int vprintf(const char *fmt, va_list ap);
int printf(const char *, ...) __printflike(1, 2);
void _setputcfn(int(*)(int));

#endif
