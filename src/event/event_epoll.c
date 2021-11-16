#include "stdafx.h"
#include "common.h"

#ifdef	HAS_EPOLL

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sys/epoll.h>
#include "event.h"
#include "event_epoll.h"

/****************************************************************************/

typedef struct EVENT_EPOLL {
	EVENT event;
	int   epfd;
	struct epoll_event *events;
	int   size;
} EVENT_EPOLL;

static void epoll_free(EVENT *ev)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

	close(ep->epfd);
	mem_free(ep->events);
	mem_free(ep);
}

static int epoll_add_read(EVENT_EPOLL *ep, FILE_EVENT *fe)
{
	struct epoll_event ee;
	int op, n;

	if ((fe->mask & EVENT_READ)) {
		return 0;
    }

	ee.events   = 0;
	ee.data.u32 = 0;
	ee.data.u64 = 0;
	ee.data.ptr = fe;

	ee.events |= EPOLLIN;
	if (fe->mask & EVENT_WRITE) {
		ee.events |= EPOLLOUT;
		op = EPOLL_CTL_MOD;
		n  = 0;
	} else {
		op = EPOLL_CTL_ADD;
		n  = 1;
	}

	if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
		fe->mask |= EVENT_READ;
		ep->event.fdcount += n;
		return 0;
	}

	if (errno != EPERM) {
		msg_error("%s(%d): epoll_ctl error %d, epfd=%d, fd=%d\n",
			__FUNCTION__, __LINE__, last_error(), ep->epfd, fe->fd);
	}
	return -1;
}

static int epoll_add_write(EVENT_EPOLL *ep, FILE_EVENT *fe)
{
	struct epoll_event ee;
	int op, n;

	ee.events   = 0;
	ee.data.u32 = 0;
	ee.data.u64 = 0;
	ee.data.ptr = fe;

	ee.events |= EPOLLOUT;

	if (fe->mask & EVENT_READ) {
		ee.events |= EPOLLIN;
		op = EPOLL_CTL_MOD;
		n  = 0;
	} else {
		op = EPOLL_CTL_ADD;
		n  = 1;
	}

	if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
		fe->mask |= EVENT_WRITE;
		ep->event.fdcount += n;
		return 0;
	}

	if (errno != EPERM) {
		msg_error("%s(%d): epoll_ctl error %d, epfd=%d, fd=%d",
			__FUNCTION__, __LINE__, last_error(), ep->epfd, fe->fd);
	}
	return -1;
}

static int epoll_del_read(EVENT_EPOLL *ep, FILE_EVENT *fe)
{
	struct epoll_event ee;
	int op, n = 0;

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.fd  = 0;
	ee.data.ptr = fe;

	if (fe->mask & EVENT_WRITE) {
		ee.events = EPOLLOUT;
		op = EPOLL_CTL_MOD;
		n  = 0;
	} else {
		op = EPOLL_CTL_DEL;
		n  = -1;
	}

	if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
		fe->mask &= ~EVENT_READ;
		ep->event.fdcount += n;
		return 0;
	}

	if (errno != EEXIST) {
		msg_error("%s(%d), epoll_ctl error: %d, epfd=%d, fd=%d",
			__FUNCTION__, __LINE__, last_error(), ep->epfd, fe->fd);
	}
	return -1;
}

static int epoll_del_write(EVENT_EPOLL *ep, FILE_EVENT *fe)
{
	struct epoll_event ee;
	int op, n;

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.fd  = 0;
	ee.data.ptr = fe;

	if (fe->mask & EVENT_READ) {
		ee.events = EPOLLIN;
		op = EPOLL_CTL_MOD;
		n  = 0;
	} else {
		op = EPOLL_CTL_DEL;
		n  = -1;
	}

	if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
		fe->mask &= ~EVENT_WRITE;
		ep->event.fdcount += n;
		return 0;
	}

	if (errno != EEXIST) {
		msg_error("%s(%d), epoll_ctl error: %d, efd=%d, fd=%d",
			__FUNCTION__, __LINE__, last_error(), ep->epfd, fe->fd);
	}
	return -1;
}

static int epoll_event_wait(EVENT *ev, int timeout)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	struct epoll_event *ee;
	FILE_EVENT *fe;
	int n, i;

	n = epoll_wait(ep->epfd, ep->events, ep->size, timeout);

	if (n < 0) {
		if (last_error() == EVENT_EINTR) {
			return 0;
		}
		msg_fatal("%s: epoll_wait error %d", __FUNCTION__, last_error());
	} else if (n == 0) {
		return 0;
	}

	for (i = 0; i < n; i++) {
		ee = &ep->events[i];
		fe = (FILE_EVENT *) ee->data.ptr;

#define EVENT_ERR	(EPOLLERR | EPOLLHUP)

		if (ee->events & (EPOLLIN | EVENT_ERR) && fe && fe->r_proc) {
			fe->r_proc(ev, fe);
		}

		if (ee->events & (EPOLLOUT | EVENT_ERR) && fe && fe->w_proc) {
			fe->w_proc(ev, fe);
		}
	}

	return n;
}

static int epoll_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	if (ev->add_read(ev, fe) == -1) {
		return -1;
	}
	if (ev->del_read(ev, fe) == -1) {
		msg_error("%s(%d): del_read failed, fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}
	return 0;
}

static long epoll_handle(EVENT *ev)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

	return ep->epfd;
}

static const char *epoll_name(void)
{
	return "epoll";
}

EVENT *event_epoll_create(int size)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) mem_calloc(1, sizeof(EVENT_EPOLL));

	ep->events = (struct epoll_event *)
		mem_malloc(sizeof(struct epoll_event) * size);
	ep->size   = size;

	ep->epfd = epoll_create(1024);
	assert(ep->epfd >= 0);

	ep->event.name   = epoll_name;
	ep->event.handle = (acl_handle_t (*)(EVENT *)) epoll_handle;
	ep->event.free   = epoll_free;

	ep->event.event_wait = epoll_event_wait;
	ep->event.checkfd    = (event_oper *) epoll_checkfd;
	ep->event.add_read   = (event_oper *) epoll_add_read;
	ep->event.add_write  = (event_oper *) epoll_add_write;
	ep->event.del_read   = (event_oper *) epoll_del_read;
	ep->event.del_write  = (event_oper *) epoll_del_write;

	return (EVENT*) ep;
}

#endif	// end HAS_EPOLL
