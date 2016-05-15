#include <microkernel.h>
#include <mrg.h>

#define CONSOLE_NAMEID squoze("console")

static void pltconsole(int indd, int outdd)
{
	int ret;
	struct sys_creat_cfg cfg;
	struct sys_poll_ior ior;
	int reqevt = evtalloc();
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.nameid = CONSOLE_NAMEID;

	ret = devcreat(&cfg, 0111, reqevt);
	if (ret < 0) {
		/* MURGIA_PANIC? Kill bootstrap most probably -- bootstrap_ctl device */
		while(1)
			printf("YES YOU NEED TO PANIC\n");
	}

	while (1) {
		int req;
		uint64_t val = 0;

		evtwait(reqevt);

		req = devpoll(&ior);
		if (req >= 0) {
			switch (ior.op) {
			case SYS_POLL_OP_OUT:
				printf("<%c>", ior.val);
				break;
			case SYS_POLL_OP_IN:
				val = squoze("Gianluca");
				break;
			default:
				break;
			}
		}
		evtclear(reqevt);

		printf("CONSOLE REQUEST!\n");
		sys_eio(req, val);
	}

	lwt_sleep();
}

int
pltconsole_process()
{
	int ret, pid, indd = 0, outdd = 0;

	/* Search for console devices */
	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		pltconsole(indd, outdd);
	} else {
		printf("Created console process with pid %d.", pid);
	}

	return ret;
}

