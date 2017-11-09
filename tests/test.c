#include <stdio.h>
#include <stdlib.h>
#include <microkernel.h>
#include <sys/param.h>
#include <mrg.h>

#define PAGE_SIZE (1L << 21)

static int _sys_write(void *cookie, const char *c, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		sys_putc(*c++);
	}
}

void inthdlr(void *v)
{
	int ret;
	struct sys_poll_ior ior;
	uint8_t *c;
	int len;
	int i;

	while ((ret = devpoll(0, &ior)) >= 0) {
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

		for (i = 0; i < 10; i++) {
			printf(".");
			sys_out(dd, 10, 'A' + i);
			//			sys_yield();
		}
		printf("done");
		sys_die(0);
	} else {
		while(1) {
			printf("pid = %d\n", pid);
			lwt_sleep();
		}
	}

}

int reqevt;

static void __req_ast(void)
{
	int ret;
	struct sys_poll_ior ior;
	char *c[13];

	while ((ret = devpoll(0, &ior)) >= 0) {
		switch (ior.op) {
		case SYS_POLL_OP_OUT:
			printf("OUT CALLED! %lx\n", ior.val);
			devread(0, ret, ior.val, 9, c);
			printf("ast called! [%p] %s\n", c, c);
			devwrite(0, ret, "Hey you!", 9, ior.val);
			break;
		default:
			break;
		}
	}

	evtclear(reqevt);
}

void
test_usrexport(void)
{
	int pid, ret, devid;
	struct sys_creat_cfg cfg;
	uint64_t nameid = squoze("test0");
	reqevt = evtalloc();

	cfg.nameid = nameid;

	evtast(reqevt, __req_ast);
	ret = devcreat(&cfg, 0111, reqevt);

	assert (ret >= 0);
	devid = ret;

	pid = sys_fork();
	assert(pid >= 0);
	if (pid == 0) {
		void *a = malloc(2 * PAGE_SIZE);
		unsigned long iova;
		DEVICE *d;

		assert(a != NULL);
		printf("Allocated %p(%l)\n", a, 2 * PAGE_SIZE);

		d = dopen("test0");
		printf("d == %p\n", d);
		assert(d != NULL);


		ret = dexport(d, a, 2 * PAGE_SIZE, &iova);
		assert(ret == 0);
		printf("iova = %lx\n", iova);

		memcpy(a, "Hi there!", 9);

		dout(d, 10, (unsigned long )iova);
		sys_yield();
		printf("%s", a);
		printf("%s", a);
		printf("%s", a);
		printf("%s", a);

	}
	lwt_sleep();
}

int
main()
{
	FILE *syscons;
	syscons = fwopen(NULL, _sys_write);
	stdout = syscons;
	stderr = syscons;
	setvbuf(syscons, NULL, _IONBF, 80);

//	test_idle_int();
	test_usrexport();
}
