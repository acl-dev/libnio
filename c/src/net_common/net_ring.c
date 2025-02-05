#include "stdafx.h"

#include "net_ring.h"

#ifndef USE_FAST_NET_RING

/* net_ring_init - initialize ring head */
void net_ring_init(NET_RING *ring)
{
	ring->pred   = ring->succ = ring;
	ring->parent = ring;
	ring->len    = 0;
}

/* net_ring_size - the entry number in the ring */

int net_ring_size(const NET_RING *ring)
{
	return ring->len;
}

/* net_ring_append - insert entry after ring head */

void net_ring_append(NET_RING *ring, NET_RING *entry)
{
	entry->succ      = ring->succ;
	entry->pred      = ring;
	entry->parent    = ring->parent;
	ring->succ->pred = entry;
	ring->succ       = entry;
	ring->parent->len++;
}

/* net_ring_prepend - insert new entry before ring head */

void net_ring_prepend(NET_RING *ring, NET_RING *entry)
{
	entry->pred      = ring->pred;
	entry->succ      = ring;
	entry->parent    = ring->parent;
	ring->pred->succ = entry;
	ring->pred       = entry;
	ring->parent->len++;
}

/* net_ring_detach - remove entry from ring */

void net_ring_detach(NET_RING *entry)
{
	NET_RING *succ, *pred;

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

/* net_ring_pop_head - pop ring's head entry out from ring */

NET_RING *net_ring_pop_head(NET_RING *ring)
{
	NET_RING *succ;

	succ = ring->succ;
	if (succ == ring) {
		return NULL;
	}

	net_ring_detach(succ);
	return succ;
}

/* net_ring_pop_tail - pop ring's tail entry out from ring */

NET_RING *net_ring_pop_tail(NET_RING *ring)
{
	NET_RING *pred;

	pred = ring->pred;
	if (pred == ring) {
		return NULL;
	}

	net_ring_detach(pred);
	return pred;
}

#endif /* USE_FAST_NET_RING */
