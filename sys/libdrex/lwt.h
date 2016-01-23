#ifndef _DREX_LWT_H
#define _DREX_LWT_H

#include <sys/types.h>
#include <sys/queue.h>
#include <setjmp.h>

struct lwt {
	jmp_buf buf;
	void *priv;

	void (*start)(void *);
	void *arg;
	void *stack;
	size_t stack_size;

	int active;
	SIMPLEQ_ENTRY(lwt) list;
};
typedef struct lwt lwt_t;


void lwt_makebuf(lwt_t *lwt, void (*start)(void *), void * arg,
		 void *stack_base, size_t stack_size);
lwt_t *lwt_create(void (*start)(void *), void *arg, size_t stack_size);

void __lwt_wake(lwt_t *lwt);

void lwt_wake(lwt_t *lwt);
void lwt_yield(void);
void lwt_sleep(void);
void lwt_exit(void);


lwt_t *lwt_getcurrent(void);
void *lwt_getprivate(void);
void lwt_setprivate(void *ptr);

#endif
