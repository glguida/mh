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

	return 0;
}

int
devpoll(struct sys_poll_ior *ior)
{

	return sys_poll(ior);
}

int
devwriospace(unsigned id, uint32_t port, uint64_t val)
{

	return sys_wriospc(id, port, val);
}

int
devraiseirq(unsigned id, unsigned irq)
{

	return sys_irq(id, irq);
}
