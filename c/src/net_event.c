#include "stdafx.h"
#include "common.h"

#include "net_event/event_epoll.h"
#include "net_event/event_kqueue.h"
#include "net_event/event_select.h"
#include "net_event/event_poll.h"
#include "net_event/event_wmsg.h"
#include "net_event/event_iocp.h"
#include "net_event.h"

NET_EVENT *net_event_create(int size, int net_event_type)
{
	NET_EVENT *ev = NULL;

	switch (net_event_type) {
	case NET_EVENT_TYPE_POLL:
#ifdef	HAS_POLL
		ev = net_poll_create(size);
#else
		net_msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	case NET_EVENT_TYPE_SELECT:
		ev = net_select_create(size);
		break;
	case NET_EVENT_TYPE_WMSG:
#ifdef	HAS_WMSG
		ev = net_wnet_msg_create(size);
#else
		net_msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	default:
#if	defined(HAS_EPOLL)
		ev = net_epoll_create(size);
#elif	defined(HAS_KQUEUE)
		ev = net_kqueue_create(size);
#elif	defined(HAS_IOCP)
		ev = net_iocp_create(size);
#else
# error	"unknown OS"
#endif
		break;
	}

	assert(ev);
	net_ring_init(&ev->events);
	ev->timeout = -1;
	ev->setsize = size;
	ev->fdcount = 0;
	ev->maxfd   = -1;
	ev->waiter  = 0;

	return ev;
}

const char *net_event_name(NET_EVENT *ev)
{
	return ev->name();
}

ssize_t net_event_size(NET_EVENT *ev)
{
	return ev->setsize;
}

void net_event_free(NET_EVENT *ev)
{
	ev->free(ev);
}

#ifdef SYS_WIN
static int checkfd(NET_EVENT *ev, NET_FILE *fe)
{
	if (getsocktype(fe->fd) >= 0) {
		return 0;
	}
	return ev->checkfd(ev, fe);
}
#else
static int checkfd(NET_EVENT *ev, NET_FILE *fe)
{
	(void) ev;
	/* If we cannot seek, it must be a pipe, socket or fifo, else it
	 * should be a file.
	 */
	if (lseek(fe->fd, (off_t) 0, SEEK_SET) == -1 && errno == ESPIPE) {
		return 0;
	} else {
		return -1;
	}
}
#endif

int net_event_add_read(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc)
{
	assert(fe);

	if (fe->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		net_msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (fe->oper & NET_EVENT_DEL_READ) {
		fe->oper &= ~NET_EVENT_DEL_READ;
	}

	if (!(fe->mask & NET_EVENT_READ)) {
		if (fe->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				fe->type = TYPE_NOSOCK;
				return 0;
			} else {
				fe->type = TYPE_SOCK;
			}
		}

		if (fe->me.parent == &fe->me) {
			net_ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= NET_EVENT_ADD_READ;
	}

	fe->r_proc = proc;
	return 1;
}

int net_event_add_write(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc)
{
	assert(fe);

	if (fe->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		net_msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (fe->oper & NET_EVENT_DEL_WRITE) {
		fe->oper &= ~NET_EVENT_DEL_WRITE;
	}

	if (!(fe->mask & NET_EVENT_WRITE)) {
		if (fe->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				fe->type = TYPE_NOSOCK;
				return 0;
			} else {
				fe->type = TYPE_SOCK;
			}
		}

		if (fe->me.parent == &fe->me) {
			net_ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= NET_EVENT_ADD_WRITE;
	}

	fe->w_proc = proc;
	return 1;
}

void net_event_del_read(NET_EVENT *ev, NET_FILE *fe)
{
	assert(fe);

	if (fe->oper & NET_EVENT_ADD_READ) {
		fe->oper &=~NET_EVENT_ADD_READ;
	}

	if (fe->mask & NET_EVENT_READ) {
		if (fe->me.parent == &fe->me) {
			net_ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= NET_EVENT_DEL_READ;
	}

	fe->r_proc  = NULL;
}

void net_event_del_write(NET_EVENT *ev, NET_FILE *fe)
{
	assert(fe);

	if (fe->oper & NET_EVENT_ADD_WRITE) {
		fe->oper &= ~NET_EVENT_ADD_WRITE;
	}

	if (fe->mask & NET_EVENT_WRITE) {
		if (fe->me.parent == &fe->me) {
			net_ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= NET_EVENT_DEL_WRITE;
	}

	fe->w_proc = NULL;
}

void net_event_close(NET_EVENT *ev, NET_FILE *fe)
{
	if (fe->mask & NET_EVENT_READ) {
		ev->del_read(ev, fe);
	}

	if (fe->mask & NET_EVENT_WRITE) {
		ev->del_write(ev, fe);
	}

	/* when one fiber add read/write and del read/write by another fiber
	 * in one loop, the fe->mask maybe be 0 and the fiber's fe maybe been
	 * added into events task list
	 */
	if (fe->me.parent != &fe->me) {
		net_ring_detach(&fe->me);
	}

	if (ev->event_fflush) {
		ev->event_fflush(ev);
	}
}

static void net_event_prepare(NET_EVENT *ev)
{
	NET_FILE *fe;
	NET_RING *next;

	while ((next = net_ring_first(&ev->events))) {
		fe = net_ring_to_appl(next, NET_FILE, me);

		if (fe->oper & NET_EVENT_DEL_READ) {
			ev->del_read(ev, fe);
		}
		if (fe->oper & NET_EVENT_DEL_WRITE) {
			ev->del_write(ev, fe);
		}
		if (fe->oper & NET_EVENT_ADD_READ) {
			ev->add_read(ev, fe);
		}
		if (fe->oper & NET_EVENT_ADD_WRITE) {
			ev->add_write(ev, fe);
		}

		net_ring_detach(next);
		fe->oper = 0;
	}

	net_ring_init(&ev->events);
}

int net_event_wait(NET_EVENT *ev, int timeout)
{
	int ret;

	if (ev->timeout < 0) {
		if (timeout < 0) {
			timeout = 100;
		}
	} else if (timeout < 0) {
		timeout = ev->timeout;
	} else if (timeout > ev->timeout) {
		timeout = ev->timeout;
	}

	/* limit the event wait time just for fiber schedule exiting
	 * quickly when no tasks left
	 */
	if (timeout > 1000 || timeout < 0) {
		timeout = 100;
	}

	net_event_prepare(ev);
	ret = ev->event_wait(ev, timeout);

	return ret;
}

