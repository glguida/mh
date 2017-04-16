#ifndef __KEYB_H__
#define __KEYB_H__

void vtty_kwait(void);
int vtty_kgetc(void);
int vtty_kgetcw(void);

#ifdef __MURGIA__
void vtty_kast(void (*func)(void));
#endif

#endif
