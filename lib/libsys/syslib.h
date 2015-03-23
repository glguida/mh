#ifndef _syslib_h_
#define _syslib_h_

#include <uk/cdefs.h>
#include <uk/types.h>
#include <uk/stdarg.h>

void putchar(int c);
void vprintf(const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
void printf(const char *fmt, ...);

#endif 
