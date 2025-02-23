#include "stdafx.h"

#include "nio_ring.h"

#ifndef USE_FAST_NIO_RING

/* nio_ring_init - initialize ring head */
void nio_ring_init(NIO_RING *ring)
{
	ring->pred   = ring->succ = ring;
	ring->parent = ring;
	ring->len    = 0;
}

/* nio_ring_size - the entry number in the ring */

int nio_ring_size(const NIO_RING *ring)
{
	return ring->len;
}

/* nio_ring_append - insert entry after ring head */

void nio_ring_append(NIO_RING *ring, NIO_RING *entry)
{
	entry->succ      = ring->succ;
	entry->pred      = ring;
	entry->parent    = ring->parent;
	ring->succ->pred = entry;
	ring->succ       = entry;
	ring->parent->len++;
}

/* nio_ring_prepend - insert new entry before ring head */

void nio_ring_prepend(NIO_RING *ring, NIO_RING *entry)
{
	entry->pred      = ring->pred;
	entry->succ      = ring;
	entry->parent    = ring->parent;
	ring->pred->succ = entry;
	ring->pred       = entry;
	ring->parent->len++;
}

/* nio_ring_detach - remove entry from ring */

void nio_ring_detach(NIO_RING *entry)
{
	NIO_RING *succ, *pred;

	if (entry->parent != entry) {
		succ = entry->succ;
		pred = entry->pred;
		if (succ && pred) {
			pred->succ = succ;
			succ->pred = pred;

			entry->parent->len--;
			entry->succ   = entry->pred = entry;
			entry->parent = entry;
			entry->len    = 0;
		}
	}
}

/* nio_ring_pop_head - pop ring's head entry out from ring */

NIO_RING *nio_ring_pop_head(NIO_RING *ring)
{
	NIO_RING *succ;

	succ = ring->succ;
	if (succ == ring) {
		return NULL;
	}

	nio_ring_detach(succ);
	return succ;
}

/* nio_ring_pop_tail - pop ring's tail entry out from ring */

NIO_RING *nio_ring_pop_tail(NIO_RING *ring)
{
	NIO_RING *pred;

	pred = ring->pred;
	if (pred == ring) {
		return NULL;
	}

	nio_ring_detach(pred);
	return pred;
}

#endif /* USE_FAST_NIO_RING */
