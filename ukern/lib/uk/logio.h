#ifndef __uk_logio_h
#define __uk_logio_h

#include <uk/cdefs.h>
#include <uk/stdarg.h>

void putc(int);
void sysputc(int);
void _setputcfn(void (*)(int), void (*)(int));

int vprintf(const char *fmt, va_list ap);
int printf(const char *, ...) __printflike(1, 2);

#ifdef __DEBUG
#define dprintf(...) do { printf(__VA_ARGS__); } while(0)
#else
#define dprintf(...)
#endif 

#endif
