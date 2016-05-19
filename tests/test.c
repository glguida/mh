#include <stdio.h>
#include <stdlib.h>
#include <microkernel.h>
#include <sys/param.h>
#include <mrg.h>

void inthdlr(void *v)
{
	int ret;
	struct sys_poll_ior ior;
	uint8_t *c;
	int len;
	int i;

	while ((ret = devpoll(&ior)) >= 0) {
		printf("ret = %d\n", ret);
		switch (ior.op) {
		case SYS_POLL_OP_OUT:
			c = &ior.val;
			len = MIN(sizeof(ior.val), (1 << ior.size));

			printf("(%d){", ior.size);
			for (i = 0; i < len; i++, c++)
				printf("%c", *c);
			printf("}");
			break;
		default:
			break;
		}
	}
}

int
test_idle_int(void)
{
	int pid, ret;
	struct sys_creat_cfg cfg;
	int reqint = intalloc();
	uint64_t nameid = 0x5005;

	cfg.nameid = nameid;

	ret = sys_creat(&cfg, reqint, 0111);
	assert(ret >= 0);

	inthandler(reqint, inthdlr, NULL);

	pid = sys_fork();
	assert(pid >= 0);
	if (pid == 0) {
		int i;
		int dd = sys_open(nameid);

		assert(dd >= 0);

		while(1) {
			printf(".");
			for(i = 0; i < 100000000; i++) printf("");
			sys_out(dd, 10, 20);
		}
		printf("done");
		sys_die(0);
	} else {
		lwt_sleep();
	}

}

int
main()
{
	test_idle_int();
}
