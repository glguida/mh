/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/null.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#include <drex/preempt.h>
#include <drex/lwt.h>
#include <drex/event.h>


/*
 * drex's own kqueues.
 *
 * One day this should become more NetBSD compatible.
 */

#define DREX_QUEUES_MAX 16

extern struct evfilter _irq_evfilter;
static struct evfilter *filters[EVFILT_NUMFILTERS] = { &_irq_evfilter, 0, };
static struct drex_queue *queues[DREX_QUEUES_MAX] = { 0, };

int
drex_kqueue(void)
{
	int i;
	struct drex_queue *ptr;

	for (i = 0; i < DREX_QUEUES_MAX; i++)
		if (queues[i] == NULL)
			break;

	if (i >= DREX_QUEUES_MAX)
		return -EBUSY;

	ptr = malloc(sizeof(struct drex_queue));
	if (ptr == NULL)
		return -ENOMEM;

	ptr->flags = 0;
	ptr->lwt = NULL;
	LIST_INIT(&ptr->events);

	queues[i] = ptr;
	return i;
}

int drex_kqueue_set_filter(unsigned fil, struct evfilter *evf)
{
	if (fil >= EVFILT_NUMFILTERS)
		return -EINVAL;

	preempt_disable();
	if (filters[fil] != NULL) {
		preempt_enable();
		return -EBUSY;
	}

	filters[fil] = evf;
	preempt_enable();
	return 0;
}

int
drex_kqueue_add(int qn, unsigned fil, uintptr_t ident)
{
	int ret;
	struct drex_queue *q;
	struct drex_event *e;

	if (qn >= DREX_QUEUES_MAX)
		return -EINVAL;

	if (fil >= EVFILT_NUMFILTERS)
		return -EINVAL;

	if (filters[fil] == NULL)
		return -EINVAL;

	
	q = queues[qn];
	if (q == NULL)
		return -ENOENT;

	/* Check event already registered */
	LIST_FOREACH(e, &q->events, queue_list)
		if ((e->filter == fil) && (e->ident == ident))
			return 0;

	e = malloc(sizeof(*e));
	if (e == NULL)
		return -ENOMEM;

	e->filter = fil;
	e->ident = ident;
	e->active = 0;
	e->queue = q;

	ret = filters[fil]->attach(e, ident);
	if (ret < 0) {
		free(e);
		return ret;
	}
	LIST_INSERT_HEAD(&q->events, e, queue_list);
	return 0;
}

int
drex_kqueue_del(int qn, unsigned filter, uintptr_t ident)
{
	return -ENOSYS;
}

int
drex_kqueue_wait(int qn, unsigned *filter, uintptr_t *ident, int poll)
{
	struct drex_queue *q;
	struct drex_event *e;

	if (qn >= DREX_QUEUES_MAX)
		return -EINVAL;

	q = queues[qn];
	if (q == NULL)
		return -ENOENT;

	assert(!q->flags & DREX_QUEUE_LWTWAIT);
	preempt_disable();
	while (1) {
		LIST_FOREACH(e, &q->events, queue_list) {
			if (e->active) {
				e->active = 0;
				preempt_enable();
				*filter = e->filter;
				*ident = e->ident;
				return 1;
			}
		}
		if (poll)
			return 0;
		/* No events. Wait */
		q->lwt = lwt_getcurrent();
		lwt_sleep();
	}
}

int _drex_kqueue_events(struct drex_events *evs, unsigned fil, int data)
{
	unsigned cnt = 0;
	struct drex_event *e;

	assert(fil < EVFILT_NUMFILTERS);

	LIST_FOREACH(e, evs, event_list) {
		if ((filters[fil]->event != NULL)
		    && !filters[fil]->event(e, fil, data))
			continue;

		/* No *event() or true */
		if (e->queue->lwt != NULL) {
			__lwt_wake(e->queue->lwt);
			e->queue->lwt = NULL;
		}
		e->active = 1;
		cnt++;
	}
	return cnt;
}
