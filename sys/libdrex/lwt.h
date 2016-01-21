#ifndef _DREX_LWT_H
#define _DREX_LWT_H

#include <sys/types.h>
#include <setjmp.h>

struct lwt {
	jmp_buf buf;
	void *priv;
	void *stack;
};
typedef struct lwt lwt_t;


void lwt_init(lwt_t *lwt);
void lwt_switch(lwt_t *lwt);
void lwt_create(lwt_t *lwt, void (*start)(void *), void * arg,
		void *priv, void *stack_base, size_t stack_size);
lwt_t *lwt_getcurrent(void);
void *lwt_getprivate(void);
void lwt_setprivate(void *ptr);

#endif
