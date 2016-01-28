#include <sys/types.h>
#include <sys/errno.h>
#include <sys/dirtio.h>
#include <assert.h>
#include <drex/lwt.h>

#include "dirtio_pipe.h"

static lwt_t *pipe_recv, *pipe_send;

unsigned qmax[PIPE_RINGS];
unsigned qsize[PIPE_RINGS];
unsigned qready[PIPE_RINGS];

static __notify_id = 0;

int
__dirtio_dev_pipe_notify(unsigned id, unsigned queue)
{
	if (queue >= PIPE_RINGS)
		return -EINVAL;

	__notify_id = id;
	
	if (queue == PIPE_RING_RECV)
		__lwt_wake(pipe_recv);
	else
		__lwt_wake(pipe_send);
	return 0;
}

void
pipedev_recv(void *arg)
{
  	unsigned num;
	struct iovec *iov;

	printf("Recv!\n");
	printf("%d: ", dioqueue_dev_getv(__notify_id, PIPE_RING_RECV, &num, &iov));
	printf("GOT %d (%p)\n", num, iov);
	lwt_exit();
}

void
pipedev_send(void *arg)
{
	unsigned num;
	struct iovec *iov;
	printf("Send!\n");
	printf("%d: ", dioqueue_dev_getv(__notify_id, PIPE_RING_SEND, &num, &iov));
	printf("GOT %d (%p)\n", num, iov);
	lwt_exit();
}

int
dirtio_dev_pipe_setup(uint64_t nameid, uint64_t deviceid, devmode_t mode)
{
	struct sys_creat_cfg cfg;
	struct dioqueue_dev *dqs;

	cfg.nameid = nameid;
	cfg.vendorid = DIRTIO_VID;
	cfg.deviceid = deviceid;


	qmax[PIPE_RING_RECV] = 1024;
	qmax[PIPE_RING_SEND] = 1024;
	pipe_recv = lwt_create(pipedev_recv, NULL, 65536);
	pipe_send = lwt_create(pipedev_send, NULL, 65536);
	assert(pipe_recv && pipe_send);

	dqs = malloc(sizeof(struct dioqueue_dev) * PIPE_RINGS);
	dqs[0].last_seen_avail = 0;
	dring_dev_init(&dqs[0].dring, 64, sizeof(struct dirtio_hdr), 1);
	dring_dev_init(&dqs[1].dring, 64,
		       sizeof(struct dirtio_hdr) + dring_size(64, 1), 1);

	dirtio_dev_init(PIPE_RINGS, __dirtio_dev_pipe_notify,
			qmax, qsize, qready, dqs);
	return dirtio_dev_creat(&cfg, mode);
}
