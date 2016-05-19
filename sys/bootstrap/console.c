#include <microkernel.h>
#include <sys/param.h> /* MIN,MAX */
#include <stdlib.h>

#include <stdio.h>
#include <mrg.h>

#define CONSOLE_NAMEID squoze("console")

#define mrg_panic(...) do { printf("UUH"); while(1); } while(0)

static int reqevt = -1;


static void consdev_putchar(uint8_t v)
{
	printf("%c", v);
}

static void __console_io_ast(void)
{
	uint64_t val = 0;	
	uint8_t *c;
	int req, ret, i, len;
	struct sys_poll_ior ior;

	while ((ret = devpoll(&ior)) >= 0) {
		printf("ret = %d\n", ret);
		switch (ior.op) {
		case SYS_POLL_OP_OUT:
			c = &ior.val;
			len = MIN(sizeof(ior.val), (1 << ior.size));

			printf("(%d){", ior.size);
			for (i = 0; i < len; i++, c++)
				consdev_putchar(*c);
			printf("}");
			break;
		default:
			break;
		}
	}
	evtclear(reqevt);
}


static void pltconsole()
{
	int ret;
	struct sys_creat_cfg cfg;
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.nameid = CONSOLE_NAMEID;

	reqevt = evtalloc();
	evtast(reqevt, __console_io_ast);

	ret = devcreat(&cfg, 0111, reqevt);
	if (ret < 0)
		mrg_panic("Cannot create console: devcreat() [%d]", ret);

	lwt_sleep();
}

int
pltconsole_process(void)
{
	int ret, pid;

	/* Search for console devices */
	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		pltconsole();
	} else {
		printf("Created console process with pid %d.", pid);
	}
	return pid;
}

