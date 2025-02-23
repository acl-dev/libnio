#include "stdafx.h"
#include "common.h"

#include "events/event_epoll.h"
#include "events/event_kqueue.h"
#include "events/event_select.h"
#include "events/event_poll.h"
#include "events/event_wmsg.h"
#include "events/event_iocp.h"
#include "event.h"

NIO_EVENT *nio_event_create(int size, int nio_event_type)
{
	NIO_EVENT *ev = NULL;

	switch (nio_event_type) {
	case NIO_EVENT_TYPE_POLL:
#ifdef	HAS_POLL
		ev = nio_poll_create(size);
#else
		nio_msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	case NIO_EVENT_TYPE_SELECT:
		ev = nio_select_create(size);
		break;
	case NIO_EVENT_TYPE_WMSG:
#ifdef	HAS_WMSG
		ev = nio_wnio_msg_create(size);
#else
		nio_msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	case NIO_EVENT_TYPE_KERNEL:
	default:
#if	defined(HAS_EPOLL)
		ev = nio_epoll_create(size);
#elif	defined(HAS_KQUEUE)
		ev = nio_kqueue_create(size);
#elif	defined(HAS_IOCP)
		ev = nio_iocp_create(size);
#else
# error	"unknown OS"
#endif
		break;
	}

	assert(ev);
	nio_ring_init(&ev->events);
	ev->timeout = -1;
	ev->setsize = (size_t) size;
	ev->fdcount = 0;
	ev->maxfd   = -1;
	ev->waiter  = 0;

	return ev;
}

const char *nio_event_name(NIO_EVENT *ev)
{
	return ev->name();
}

size_t nio_event_size(NIO_EVENT *ev)
{
	return ev->setsize;
}

void nio_event_free(NIO_EVENT *ev)
{
	ev->free(ev);
}

#ifdef SYS_WIN
static int checkfd(NIO_EVENT *ev, NIO_FILE *fe)
{
	if (getsocktype(fe->fd) >= 0) {
		return 0;
	}
	return ev->checkfd(ev, fe);
}
#else
static int checkfd(NIO_EVENT *ev, NIO_FILE *fe)
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

int nio_event_add_read(NIO_EVENT *ev, NIO_FILE *fe, nio_event_proc *proc)
{
	assert(fe);

	if (((NIO_FILE_*) fe)->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		nio_msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (((NIO_FILE_*) fe)->oper & NIO_EVENT_DEL_READ) {
		((NIO_FILE_*) fe)->oper &= ~NIO_EVENT_DEL_READ;
	}

	if (!(((NIO_FILE_*) fe)->mask & NIO_EVENT_READ)) {
		if (((NIO_FILE_*) fe)->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				((NIO_FILE_*) fe)->type = TYPE_NOSOCK;
				return 0;
			} else {
				((NIO_FILE_*) fe)->type = TYPE_SOCK;
			}
		}

		if (((NIO_FILE_*) fe)->me.parent == &((NIO_FILE_*) fe)->me) {
			nio_ring_prepend(&ev->events, &((NIO_FILE_*) fe)->me);
		}

		((NIO_FILE_*) fe)->oper |= NIO_EVENT_ADD_READ;
	}

	((NIO_FILE_*) fe)->r_proc = proc;
	return 1;
}

int nio_event_add_write(NIO_EVENT *ev, NIO_FILE *fe, nio_event_proc *proc)
{
	assert(fe);

	if (((NIO_FILE_*) fe)->type == TYPE_NOSOCK) {
		return 0;
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		nio_msg_error("fd: %d >= setsize: %d", fe->fd, (int) ev->setsize);
		return 0;
	}

	if (((NIO_FILE_*) fe)->oper & NIO_EVENT_DEL_WRITE) {
		((NIO_FILE_*) fe)->oper &= ~NIO_EVENT_DEL_WRITE;
	}

	if (!(((NIO_FILE_*) fe)->mask & NIO_EVENT_WRITE)) {
		if (((NIO_FILE_*) fe)->type == TYPE_NONE) {
			if (checkfd(ev, fe) == -1) {
				((NIO_FILE_*) fe)->type = TYPE_NOSOCK;
				return 0;
			} else {
				((NIO_FILE_*) fe)->type = TYPE_SOCK;
			}
		}

		if (((NIO_FILE_*) fe)->me.parent == &((NIO_FILE_*) fe)->me) {
			nio_ring_prepend(&ev->events, &((NIO_FILE_*) fe)->me);
		}

		((NIO_FILE_*) fe)->oper |= NIO_EVENT_ADD_WRITE;
	}

	((NIO_FILE_*) fe)->w_proc = proc;
	return 1;
}

void nio_event_del_read(NIO_EVENT *ev, NIO_FILE *fe)
{
	assert(fe);

	if (((NIO_FILE_*) fe)->oper & NIO_EVENT_ADD_READ) {
		((NIO_FILE_*) fe)->oper &=~NIO_EVENT_ADD_READ;
	}

	if (((NIO_FILE_*) fe)->mask & NIO_EVENT_READ) {
		if (((NIO_FILE_*) fe)->me.parent == &((NIO_FILE_*) fe)->me) {
			nio_ring_prepend(&ev->events, &((NIO_FILE_*) fe)->me);
		}

		((NIO_FILE_*) fe)->oper |= NIO_EVENT_DEL_READ;
	}

	((NIO_FILE_*) fe)->r_proc  = NULL;
}

void nio_event_del_write(NIO_EVENT *ev, NIO_FILE *fe)
{
	assert(fe);

	if (((NIO_FILE_*) fe)->oper & NIO_EVENT_ADD_WRITE) {
		((NIO_FILE_*) fe)->oper &= ~NIO_EVENT_ADD_WRITE;
	}

	if (((NIO_FILE_*) fe)->mask & NIO_EVENT_WRITE) {
		if (((NIO_FILE_*) fe)->me.parent == &((NIO_FILE_*) fe)->me) {
			nio_ring_prepend(&ev->events, &((NIO_FILE_*) fe)->me);
		}

		((NIO_FILE_*) fe)->oper |= NIO_EVENT_DEL_WRITE;
	}

	((NIO_FILE_*) fe)->w_proc = NULL;
}

void nio_event_close(NIO_EVENT *ev, NIO_FILE *fe)
{
	if (((NIO_FILE_*) fe)->mask & NIO_EVENT_READ) {
		ev->del_read(ev, ((NIO_FILE_*) fe));
	}

	if (((NIO_FILE_*) fe)->mask & NIO_EVENT_WRITE) {
		ev->del_write(ev, ((NIO_FILE_*) fe));
	}

	/* When one fiber add read/write and del read/write by another fiber
	 * in one loop, the fe->mask maybe be 0 and the fiber's fe maybe been
	 * added into events task list.
	 */
	if (((NIO_FILE_*) fe)->me.parent != &((NIO_FILE_*) fe)->me) {
		nio_ring_detach(&((NIO_FILE_*) fe)->me);
	}

	// Force to delete read/write event monitor for the given fd.
	if (ev->event_fflush) {
		ev->event_fflush(ev);
	}
}

static void nio_event_prepare(NIO_EVENT *ev)
{
	NIO_FILE_ *fe;
	NIO_RING *next;

	while ((next = nio_ring_first(&ev->events))) {
		fe = nio_ring_to_appl(next, NIO_FILE_, me);

		if (fe->oper & NIO_EVENT_DEL_READ) {
			ev->del_read(ev, fe);
		}
		if (fe->oper & NIO_EVENT_DEL_WRITE) {
			ev->del_write(ev, fe);
		}
		if (fe->oper & NIO_EVENT_ADD_READ) {
			ev->add_read(ev, fe);
		}
		if (fe->oper & NIO_EVENT_ADD_WRITE) {
			ev->add_write(ev, fe);
		}

		nio_ring_detach(next);
		fe->oper = 0;
	}

	nio_ring_init(&ev->events);
}

int nio_event_wait2(NIO_EVENT *ev, int timeout, void (*before_wait)(void *), void *ctx)
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

	/* Limit the event wait time just for fiber schedule exiting
	 * quickly when no tasks left.
	 */
	if (timeout > 1000 || timeout < 0) {
		timeout = 100;
	}

	nio_event_prepare(ev);

	if (before_wait) {
		before_wait(ctx);
	}

	ret = ev->event_wait(ev, timeout);

	return ret;
}

int nio_event_wait(NIO_EVENT *ev, int timeout)
{
	return nio_event_wait2(ev, timeout, NULL, NULL);
}

void nio_event_debug(int on)
{
	nio_msg_stdout(on);
	printf("NIO_FILE's size: %zd bytes\r\n", sizeof(NIO_FILE));
	printf("NIO_FILE_'s size: %zd bytes\r\n", sizeof(NIO_FILE_));
	printf("NIO_RING's size: %zd bytes\r\n", sizeof(NIO_RING));
}
