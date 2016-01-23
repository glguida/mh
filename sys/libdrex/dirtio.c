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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/dirtio.h>
#include <drex/preempt.h>
#include <microkernel.h>

int dirtio_open(struct dirtio_desc *dio, uint64_t nameid,
		struct dirtio_hdr *hdr)
{
	int id, ret, eioirq;
	struct sys_creat_cfg cfg;

	id = sys_open(nameid);
	if (id < 0)
		return id;

	eioirq = irqalloc();
	ret = sys_mapirq(id, 0, eioirq);
	printf("eioirq = %d\n", eioirq);
	if (ret) {
		irqfree(eioirq);
		return ret;
	}
	
	ret = sys_readcfg(id, &cfg);
	if (ret)
		return ret;

	if (cfg.vendorid != DIRTIO_VID)
		return -EINVAL;
	
	dio->id = id;
	dio->hdr = hdr;

	dio->nameid = cfg.nameid;
	dio->vendorid = cfg.vendorid;
	dio->deviceid = cfg.deviceid;
	dio->eioirq = eioirq;

	return 0;
}

int dirtio_mmio_inw(struct dirtio_desc *dio, uint32_t port,
		    uint8_t queue, uint64_t *val)
{
	int ret;

	ret = sys_out(dio->id, PORT_DIRTIO_IN | (port << 8) | queue, 0);
	if (ret)
		return ret;
	printf("Waiting %d!\n", dio->eioirq);
	irqwait(dio->eioirq);
	*val = dio->hdr->ioval;
	return 0;
}

int dirtio_mmio_out(struct dirtio_desc *dio, uint32_t port,
		    uint8_t queue, uint64_t val)
{
	int ret;

	ret = sys_out(dio->id, ((port << 8) | queue)  & ~PORT_DIRTIO_IN, val);
	return ret;
}

int dirtio_mmio_outw(struct dirtio_desc *dio, uint32_t port,
		     uint8_t queue, uint64_t val)
{
	int ret;

	ret = sys_out(dio->id, ((port << 8) | queue)  & ~PORT_DIRTIO_IN, val);
	if (ret)
		return ret;
	irqwait(dio->eioirq);
	return 0;
}
