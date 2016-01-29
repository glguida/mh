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


#ifndef __drex_event_h_
#define __drex_event_h_

#include <sys/queue.h>
#include <drex/lwt.h>

struct drex_queue;

struct drex_event {
	unsigned filter;
	uintptr_t ident;
	int data;

	int active;
	struct drex_queue *queue;
	LIST_ENTRY(drex_event) event_list;
	LIST_ENTRY(drex_event) queue_list;
};

LIST_HEAD(drex_events, drex_event);

struct drex_queue {
#define DREX_QUEUE_LWTWAIT 1
	unsigned flags;
	lwt_t *lwt;
	LIST_HEAD(, drex_event) events;
};

#define EVFILT_DREX_IRQ 0
#define EVFILT_DIRTIO_PIPE 1
#define EVFILT_NUMFILTERS 2

struct evfilter {
	int (*attach)(struct drex_event *e, uintptr_t ident);
	int (*detach)(struct drex_event *e, uintptr_t ident);

	int (*event)(struct drex_event *e, unsigned evfilt, int data);
};

int _drex_kqueue_events(struct drex_events *evs, uintptr_t ident, int data);

int drex_kqueue_setup(unsigned fil, struct evfilter *evf);

int drex_kqueue(void);
int drex_kqueue_add(int qn, unsigned fil, uintptr_t ident);
int drex_kqueue_del(int qn, unsigned fil, uintptr_t ident);
int drex_kqueue_wait(int qn, unsigned *filter, uintptr_t *ident, int poll);

#endif
