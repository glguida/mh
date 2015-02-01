#ifndef __libukern_h
#define __libukern_h

#include <uk/cdefs.h>
#include <uk/stdarg.h>

#define panic(_s) do { printf("PANIC: %s\n", _s); while(1); } while(0)

int (*putc)(int);
void *memset(void *b, int c, size_t len);
void *memcpy(void *d, void *s, size_t len);
int memcmp(void *s1, void *s2, size_t len);
int vprintf(const char *fmt, va_list ap);
int printf(const char *, ...) __printflike(1, 2);
void _setputcfn(int(*)(int));
int fls(int);
int ffs(int);

int _setjmp(jmp_buf);
void _longjmp(jmp_buf, int);
void _setupjmp(jmp_buf, void (*)(void), void *);

#ifdef __DEBUG
#define dprintf(...) do { printf(__VA_ARGS__); } while(0)
#else
#define dprintf(...) 
#endif

#define offsetof(type, member)	__builtin_offsetof(type, member)

#endif
