#ifndef __stdarg_h
#define __stdarg_h

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start((ap), (last))
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

#endif
