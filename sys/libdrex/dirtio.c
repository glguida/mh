#include <sys/null.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/dirtio.h>
#include <microkernel.h>

int dirtio_open(struct dirtio_desc *dio, uint64_t nameid,
		struct dirtio_hdr *hdr)
{
	int id, ret;
	struct sys_creat_cfg cfg;

	id = sys_open(nameid);
	if (id < 0)
		return id;

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
	return 0;
}

void dirtio_mmio_wait(struct dirtio_desc *dio)
{
	/* XXX: CHECK ACTUALLY WAITING FOR IT */
	sys_wait();
}

int dirtio_mmio_inw(struct dirtio_desc *dio, uint32_t port, uint32_t *val)
{
	int ret;

	ret = sys_out(dio->id, PORT_DIRTIO_IN | port, 0);
	if (ret)
		return ret;
	dirtio_mmio_wait(dio);
	*val = dio->hdr->ioval;
	return 0;
}

int dirtio_mmio_out(struct dirtio_desc *dio, uint32_t port, uint32_t val)
{
	int ret;

	ret = sys_out(dio->id, port & ~PORT_DIRTIO_IN, val);
	return ret;
}

int dirtio_mmio_outw(struct dirtio_desc *dio, uint32_t port, uint32_t val)
{
	int ret;

	ret = dirtio_mmio_out(dio, port, val);
	if (ret)
		return ret;
	dirtio_mmio_wait(dio);
	return 0;
}
