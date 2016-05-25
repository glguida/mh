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


#include <machine/vmparam.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/bitops.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/dirtio.h>
#include <drex/drex.h>

#include "dirtio_pipe.h"

#define PIPE_BUFSZ (128*1024)

#define HDRMAXSZ 4096
#define NBUFS ((PAGE_SIZE - HDRMAXSZ)/PIPE_BUFSZ)
#define BUFBMAPSZ ((NBUFS + 63)/64)

struct diodevice *dirtio_pipe_open(uint64_t nameid)
{
	uint8_t *buf;
	struct diodevice *dc;
	size_t dcsz = sizeof(*dc) + PIPE_RINGS * sizeof(struct dioqueue);

	dc = malloc(dcsz);
	assert(dc);
	buf = drex_mmap(NULL, PAGE_SIZE,
			PROT_READ|PROT_WRITE, MAP_ANON, -1, 0);
	assert(buf != MAP_FAILED);
	/* XXX: ENSURE MAPPED IN MEMORY WHEN EXPORT HAPPENS IN OPEN */
	memset(buf, 0, HDRMAXSZ);

	dc->id = dirtio_desc_open(nameid, (void *)buf, &dc->desc);
	if (dc->id < 0) {
		drex_munmap(buf, PAGE_SIZE);
		return NULL;
	}

#if 0
	/* Check device actually a pipe */
	if (dc->desc->classid != DIRTIO_CID_PIPE) {
		dirtio_close(dc->desc);
		drex_munmap(buf, PAGE_SIZE);
		return NULL;
	}
#endif

	dc->buf = buf;

	/* Setup dirt queues */
	buf += sizeof(struct dirtio_hdr);
	dioqueue_init(dc->dqueues + 0, 64, buf, dc);
	buf += dring_size(64, 1);
	dioqueue_init(dc->dqueues + 1, 64, buf, dc);
	buf += dring_size(64, 1);
	assert((size_t)((uintptr_t)buf - (uintptr_t)dc->buf) < HDRMAXSZ);

	dc->hdrmax = HDRMAXSZ;
	dc->bufsz = PIPE_BUFSZ;
	dc->nqueues = PIPE_RINGS;
	dc->bmap = malloc(sizeof(uint64_t) * DIRTIO_BUFBMAPSZ(dc));
	assert(dc->bmap);
	memset(dc->bmap, 0, sizeof(uint64_t) * DIRTIO_BUFBMAPSZ(dc));
	dc->bmap[DIRTIO_BUFBMAPSZ(dc) - 1] = DIRTIO_BUFBMAPMASK(dc);

	return dc;
}
