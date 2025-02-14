#include "stdafx.h"
#include "common.h"

#ifdef HAS_SELECT

#include "event_select.h"

typedef struct EVENT_SELECT {
	NET_EVENT  event;
	fd_set rset;
	fd_set wset;
	fd_set xset;
	NET_FILE_ **files;
	int    size;
	int    count;
	socket_t maxfd;
	int    dirty;
	NET_ARRAY *ready;
} EVENT_SELECT;

static void select_free(NET_EVENT *ev)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	mem_free(es->files);
	net_array_free(es->ready, NULL);
	mem_free(es);
}

static int select_add_read(EVENT_SELECT *es, NET_FILE_ *fe)
{
	if (FD_ISSET(fe->fd, &es->wset) || FD_ISSET(fe->fd, &es->rset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
		if (fe->fd > es->maxfd) {
			es->maxfd = fe->fd;
		}
		es->event.fdcount++;
	}

	fe->mask |= NET_EVENT_READ;
	FD_SET(fe->fd, &es->rset);
	return 0;
}

static int select_add_write(EVENT_SELECT *es, NET_FILE_ *fe)
{
	if (FD_ISSET(fe->fd, &es->rset) || FD_ISSET(fe->fd, &es->wset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
		if (fe->fd > es->maxfd) {
			es->maxfd = fe->fd;
		}
		es->event.fdcount++;
	}

	fe->mask |= NET_EVENT_WRITE;
	FD_SET(fe->fd, &es->wset);
	return 0;
}

static int select_del_read(EVENT_SELECT *es, NET_FILE_ *fe)
{
	assert(fe->id >= 0 && fe->id < es->count);
	if (FD_ISSET(fe->fd, &es->rset)) {
		FD_CLR(fe->fd, &es->rset);
	}
	if (!FD_ISSET(fe->fd, &es->wset)) {
		FD_CLR(fe->fd, &es->xset);
		if (fe->id < --es->count) {
			es->files[fe->id] = es->files[es->count];
			es->files[fe->id]->id = fe->id;
		}
		fe->id = -1;
		if (fe->fd == es->maxfd) {
			es->dirty = 1;
		}
		es->event.fdcount--;
	}
	fe->mask &= ~NET_EVENT_READ;
	return 0;
}

static int select_del_write(EVENT_SELECT *es, NET_FILE_ *fe)
{
	assert(fe->id >= 0 && fe->id < es->count);
	if (FD_ISSET(fe->fd, &es->wset)) {
		FD_CLR(fe->fd, &es->wset);
	}
	if (!FD_ISSET(fe->fd, &es->rset)) {
		FD_CLR(fe->fd, &es->xset);
		if (fe->id < --es->count) {
			es->files[fe->id] = es->files[es->count];
			es->files[fe->id]->id = fe->id;
		}
		fe->id = -1;
		if (fe->fd == es->maxfd) {
			es->dirty = 1;
		}
		es->event.fdcount--;
	}
	fe->mask &= ~NET_EVENT_WRITE;
	return 0;
}

static int select_event_wait(NET_EVENT *ev, int timeout)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	fd_set rset = es->rset, wset = es->wset, xset = es->xset;
	struct timeval tv, *tp;
	ITER   iter;
	int n, i;

	if (timeout >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tp = &tv;
	} else {
		tp = NULL;
	}

#ifdef SYS_WIN
	if (ev->fdcount == 0) {
		Sleep(timeout);
		return 0;
	}

	n = select(0, &rset, &wset, &xset, tp);
#else
	if (es->dirty) {
		es->maxfd = -1;
		for (i = 0; i < es->count; i++) {
			NET_FILE_ *fe = es->files[i];
			if (fe->fd > es->maxfd) {
				es->maxfd = fe->fd;
			}
		}
	}
	n = select(es->maxfd + 1, &rset, 0, &xset, tp);
#endif
	if (n < 0) {
		if (net_last_error() == EVENT_EINTR) {
			return 0;
		}
		net_msg_fatal("%s: select error %d", __FUNCTION__, net_last_error());
	} else if (n == 0) {
		return 0;
	}

	for (i = 0; i < es->count; i++) {
		NET_FILE_ *fe = es->files[i];
		net_array_append(es->ready, fe);
	}

	foreach(iter, es->ready) {
		NET_FILE_ *fe = (NET_FILE_ *) iter.data;

		if (FD_ISSET(fe->fd, &xset)) {
			if (FD_ISSET(fe->fd, &es->rset) && fe->r_proc) {
				fe->r_proc(ev, (NET_FILE*) fe);
			}
			if (FD_ISSET(fe->fd, &es->wset) && fe->w_proc) {
				fe->w_proc(ev, (NET_FILE*) fe);
			}
		} else {
			if (FD_ISSET(fe->fd, &rset) && fe->r_proc) {
				fe->r_proc(ev, (NET_FILE*) fe);
			}
			if (FD_ISSET(fe->fd, &wset) && fe->w_proc) {
				fe->w_proc(ev, (NET_FILE*) fe);
			}
		}
	}

	net_array_clean(es->ready, NULL);

	return n;
}

static int select_checkfd(NET_EVENT *ev UNUSED, NET_FILE_ *fe UNUSED)
{
	return -1;
}

static net_handle_t select_handle(NET_EVENT *ev)
{
	(void) ev;
	return (net_handle_t)-1;
}

static const char *select_name(void)
{
	return "select";
}

NET_EVENT *net_select_create(int size)
{
	EVENT_SELECT *es = (EVENT_SELECT *) mem_calloc(1, sizeof(EVENT_SELECT));

	// override size with system open limit setting
	size      = net_open_limit(0);
	es->maxfd = -1;
	es->dirty = 0;
	es->files = (NET_FILE_**) mem_calloc(size, sizeof(NET_FILE_*));
	es->size  = size;
	es->ready = net_array_create(100);
	es->count = 0;
	FD_ZERO(&es->rset);
	FD_ZERO(&es->wset);
	FD_ZERO(&es->xset);

	es->event.name   = select_name;
	es->event.handle = select_handle;
	es->event.free   = select_free;

	es->event.event_wait = select_event_wait;
	es->event.checkfd    = (net_event_oper *) select_checkfd;
	es->event.add_read   = (net_event_oper *) select_add_read;
	es->event.add_write  = (net_event_oper *) select_add_write;
	es->event.del_read   = (net_event_oper *) select_del_read;
	es->event.del_write  = (net_event_oper *) select_del_write;

	return (NET_EVENT*) es;
}

#endif
