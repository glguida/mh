#include <mrg.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdlib.h>

#define perror(...) printf(__VA_ARGS__)
#warning add perror

struct _DEVICE {
#define FLAG_HAS_EIO 1
	unsigned flags;
	int dd;
	int eio;
};

DEVICE *
dopen(char *devname, devmode_t mode)
{
	DEVICE *d;
	int ret, dd;
	uint64_t nameid;
	struct sys_rdcfg_cfg cfg;

	d = malloc(sizeof(*d));
	if (d == NULL)
		return NULL;
	memset(d, 0, sizeof(*d));

	nameid = squoze(devname);
	dd = sys_open(nameid);
	if (dd < 0) {
		perror("open(%s)", devname);
		free(d);
		return NULL;
	}
	d->dd = dd;

	ret = sys_rdcfg(dd, &cfg);
	if (ret < 0) {
		perror("rdcfg(%s)", devname);
		sys_close(dd);
		free(d);
		return ret;
	}

	if (cfg.eio != (uint8_t)-1) {
		int eio = intalloc();

		ret = sys_mapirq(dd, cfg.eio, eio);
		if (ret < 0) {
			perror("mapirq(%s, %d, %d)",
			       devname, cfg.eio, eio);
			sys_close(dd);
			free(d);
			return ret;
		}

		d->flags |= FLAG_HAS_EIO;
		d->eio = eio;
	}

	printf("Opened device %p (%d): %d -> %d with eio: %d (flags: %lx)\n",
	       d, d->dd, cfg.eio, d->eio, d->flags);

	return d;
}



void
dclose(DEVICE *d)
{
	sys_close(d->dd);
}
