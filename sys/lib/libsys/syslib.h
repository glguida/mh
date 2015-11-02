#ifndef _syslib_h_
#define _syslib_h_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stdarg.h>

void putchar(int c);
void vprintf(const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
void printf(const char *fmt, ...);

#endif 
