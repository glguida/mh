#ifndef __setjmp_h
#define __setjmp_h

int _setjmp(jmp_buf);
void _longjmp(jmp_buf, int);
void _setupjmp(jmp_buf, void (*)(void), void *);

#endif
