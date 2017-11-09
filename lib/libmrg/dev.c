#include <mrg.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/errno.h>

static void __req_handler(int reqint, void *arg)
{
	int reqevt = (int)(uintptr_t)arg;

	__evtset(reqevt);
}

int
devcreat(struct sys_creat_cfg *cfg, devmode_t mode, int evt)
{
	int reqint = intalloc();
	int ret;

	inthandler(reqint, __req_handler, (void *)(uintptr_t)evt);

	ret = sys_creat(cfg, reqint, mode);
	if (ret < 0) {
		intfree(reqint);
		return ret;
	}

	return ret;
}

int
devpoll(unsigned did, struct sys_poll_ior *ior)
{

	return sys_poll(did, ior);
}

int
devwriospace(unsigned did, unsigned id, uint32_t port, uint64_t val)
{

	return sys_wriospc(did, id, port, val);
}

int
devraiseirq(unsigned did, unsigned id, unsigned irq)
{

	return sys_irq(did, id, irq);
}

int
devread(unsigned did, unsigned id, u_long iova, size_t sz, void *va)
{
	return sys_read(did, id, iova, sz, (unsigned long) va);
}

int
devwrite(unsigned did, unsigned id, void *va, size_t sz, u_long iova)
{
	return sys_write(did, id, (unsigned long) va, sz, iova);
}
