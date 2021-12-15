#include "stdafx.h"
#include "common.h"

#include "event/event_epoll.h"
#include "event/event_kqueue.h"
#include "event/event_select.h"
#include "event/event_poll.h"
#include "event/event_wmsg.h"
#include "event/event_iocp.h"
#include "event.h"

EVENT *event_create(int size, int event_type)
{
	EVENT *ev = NULL;

	switch (event_type) {
	case EVENT_TYPE_POLL:
#ifdef	HAS_POLL
		ev = event_poll_create(size);
#else
		msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	case EVENT_TYPE_SELECT:
		ev = event_select_create(size);
		break;
	case EVENT_TYPE_WMSG:
#ifdef	HAS_WMSG
		ev = event_wmsg_create(size);
#else
		msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	default:
#if	defined(HAS_EPOLL)
		ev = event_epoll_create(size);
#elif	defined(HAS_KQUEUE)
		ev = event_kqueue_create(size);
#elif	defined(HAS_IOCP)
		ev = event_iocp_create(size);
#else
#error	"unknown OS"
#endif
		break;
	}

	assert(ev);
	ring_init(&ev->events);
	ev->timeout = -1;
	ev->setsize = size;
	ev->fdcount = 0;
	ev->maxfd   = -1;
	ev->waiter  = 0;

	return ev;
}

const char *event_name(EVENT *ev)
{
	return ev->name();
}

ssize_t event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
	ev->free(ev);
}

#ifdef SYS_WIN
static int checkfd(EVENT *ev, FILE_EVENT *fe)
{
	if (getsocktype(fe->fd) >= 0) {
		return 0;
	}
	return ev->checkfd(ev, fe);
}
#else
static int checkfd(EVENT *ev, FILE_EVENT *fe)
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

int event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	assert(fe);

	if (fe->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (fe->oper & EVENT_DEL_READ) {
		fe->oper &= ~EVENT_DEL_READ;
	}

	if (!(fe->mask & EVENT_READ)) {
		if (fe->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				fe->type = TYPE_NOSOCK;
				return 0;
			} else {
				fe->type = TYPE_SOCK;
			}
		}

		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_ADD_READ;
	}

	fe->r_proc = proc;
	return 1;
}

int event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	assert(fe);

	if (fe->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (fe->oper & EVENT_DEL_WRITE) {
		fe->oper &= ~EVENT_DEL_WRITE;
	}

	if (!(fe->mask & EVENT_WRITE)) {
		if (fe->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				fe->type = TYPE_NOSOCK;
				return 0;
			} else {
				fe->type = TYPE_SOCK;
			}
		}

		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_ADD_WRITE;
	}

	fe->w_proc = proc;
	return 1;
}

void event_del_read(EVENT *ev, FILE_EVENT *fe)
{
	assert(fe);

	if (fe->oper & EVENT_ADD_READ) {
		fe->oper &=~EVENT_ADD_READ;
	}

	if (fe->mask & EVENT_READ) {
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_DEL_READ;
	}

	fe->r_proc  = NULL;
}

void event_del_write(EVENT *ev, FILE_EVENT *fe)
{
	assert(fe);

	if (fe->oper & EVENT_ADD_WRITE) {
		fe->oper &= ~EVENT_ADD_WRITE;
	}

	if (fe->mask & EVENT_WRITE) {
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_DEL_WRITE;
	}

	fe->w_proc = NULL;
}

void event_close(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->mask & EVENT_READ) {
		ev->del_read(ev, fe);
	}

	if (fe->mask & EVENT_WRITE) {
		ev->del_write(ev, fe);
	}

	/* when one fiber add read/write and del read/write by another fiber
	 * in one loop, the fe->mask maybe be 0 and the fiber's fe maybe been
	 * added into events task list
	 */
	if (fe->me.parent != &fe->me) {
		ring_detach(&fe->me);
	}

	if (ev->event_fflush) {
		ev->event_fflush(ev);
	}
}

static void event_prepare(EVENT *ev)
{
	FILE_EVENT *fe;
	RING *next;

	while ((next = ring_first(&ev->events))) {
		fe = ring_to_appl(next, FILE_EVENT, me);

		if (fe->oper & EVENT_DEL_READ) {
			ev->del_read(ev, fe);
		}
		if (fe->oper & EVENT_DEL_WRITE) {
			ev->del_write(ev, fe);
		}
		if (fe->oper & EVENT_ADD_READ) {
			ev->add_read(ev, fe);
		}
		if (fe->oper & EVENT_ADD_WRITE) {
			ev->add_write(ev, fe);
		}

		ring_detach(next);
		fe->oper = 0;
	}

	ring_init(&ev->events);
}

int event_wait(EVENT *ev, int timeout)
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

	event_prepare(ev);
	ret = ev->event_wait(ev, timeout);

	return ret;
}

