#include <mrg.h>
#include <sys/bitops.h>


#define MAXEVTQWORD 3
#define MAXEVT (64 * MAXEVTQWORD)
static uint64_t alloc_evts[MAXEVTQWORD] = { 0, }; 
static uint64_t set_evts[MAXEVTQWORD] = {0, };
static lwt_t *wait_evts[MAXEVT] = {0, };

int evtalloc(void)
{
	int i, evt = -1;;

	preempt_disable();
	for (i = 0; i < MAXEVTQWORD; i++) {
		if (alloc_evts[i] != ~(uint64_t)0) {
			evt = ffs64(~alloc_evts[i]) -1;
			alloc_evts[i] |= ((uint64_t)1 << evt);
			set_evts[i] &= ~((uint64_t)1 << evt);
			wait_evts[evt] = NULL;
			preempt_enable();
			return evt + i * 64;
		}
	}
	preempt_enable();

	assert(0 && "evt exhausted");
	return evt;
}

void __evtset(int evt)
{
	int i = evt / 64, r = evt % 64;

	assert(evt < MAXEVT);

	/* Interrupt context */
	set_evts[i] |= ((uint64_t)1 << r);
	if (wait_evts[evt] != NULL)
		__lwt_wake(wait_evts[evt]);
}

void evtwait(int evt)
{
	int i = evt / 64, r = evt % 64;

	assert(evt < MAXEVT);

	preempt_disable();
	/* Check event is already set. */
	if (set_evts[i] & ((uint64_t)1 << r))
		return;

	/* XXX: add to queue */
	assert(wait_evts[MAXEVT] == NULL);
	wait_evts[evt] = lwt_getcurrent();
	lwt_sleep();
	preempt_enable();
}

void evtclear(int evt)
{
  	int i = evt / 64, r = evt % 64;


	assert(evt < MAXEVT);

	preempt_disable();
	set_evts[i] &= ~((uint64_t)1 << r);
	preempt_enable();
}
