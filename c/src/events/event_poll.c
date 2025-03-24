#include "stdafx.h"

#ifdef HAS_POLL

#include "nio/nio_iostuff.h"
#include "common.h"
#include "event_poll.h"

typedef struct EVENT_POLL {
    NIO_EVENT  event;
    NIO_FILE_ **files;
    int    size;
    int    count;
    struct pollfd *pfds;
    NIO_ARRAY *ready;
} EVENT_POLL;

static void poll_free(NIO_EVENT *ev) {
    EVENT_POLL *ep = (EVENT_POLL *) ev;

    mem_free(ep->files);
    mem_free(ep->pfds);
    nio_array_free(ep->ready, NULL);
    mem_free(ep);
}

static int poll_add_read(EVENT_POLL *ep, NIO_FILE_ *fe) {
    struct pollfd *pfd;

    if (fe->id == -1) {
        assert(ep->count < ep->size);
        fe->id = ep->count++;
    }

    pfd = &ep->pfds[fe->id];

    if (pfd->events & (POLLIN | POLLOUT)) {
        assert(ep->files[fe->id] == fe);
    } else {
        pfd->events       = 0;
        pfd->fd           = fe->fd;
        pfd->revents      = 0;
        ep->files[fe->id] = fe;
        ep->event.fdcount++;
    }

    fe->mask    |= NIO_EVENT_READ;
    pfd->events |= POLLIN;
    return 0;
}

static int poll_add_write(EVENT_POLL *ep, NIO_FILE_ *fe) {
    struct pollfd *pfd = (fe->id >= 0 && fe->id < ep->count)
        ? &ep->pfds[fe->id] : NULL;

    if (fe->id == -1) {
        assert(ep->count < ep->size);
        fe->id = ep->count++;
    }

    pfd = &ep->pfds[fe->id];

    if (pfd->events & (POLLIN | POLLOUT)) {
        assert(ep->files[fe->id] == fe);
    } else {
        pfd->events       = 0;
        pfd->fd           = fe->fd;
        pfd->revents      = 0;
        ep->files[fe->id] = fe;
        ep->event.fdcount++;
    }

    fe->mask    |= NIO_EVENT_WRITE;
    pfd->events |= POLLOUT;
    return 0;
}

static int poll_del_read(EVENT_POLL *ep, NIO_FILE_ *fe) {
    struct pollfd *pfd;

    assert(fe->id >= 0 && fe->id < ep->count);
    pfd = &ep->pfds[fe->id];
    assert(pfd);

    if (pfd->events & POLLIN) {
        pfd->events &= ~POLLIN;
    }
    if (!(pfd->events & POLLOUT)) {
        if (fe->id < --ep->count) {
            ep->pfds[fe->id]      = ep->pfds[ep->count];
            ep->files[fe->id]     = ep->files[ep->count];
            ep->files[fe->id]->id = fe->id;
        }
        ep->pfds[ep->count].fd      = -1;
        ep->pfds[ep->count].events  = 0;
        ep->pfds[ep->count].revents = 0;
        fe->id = -1;
        ep->event.fdcount--;
    }
    fe->mask &= ~NIO_EVENT_READ;
    return 0;
}

static int poll_del_write(EVENT_POLL *ep, NIO_FILE_ *fe) {
    struct pollfd *pfd;

    assert(fe->id >= 0 && fe->id < ep->count);
    pfd = &ep->pfds[fe->id];
    assert(pfd);

    if (pfd->events & POLLOUT) {
        pfd->events &= ~POLLOUT;
    }
    if (!(pfd->events & POLLIN)) {
        if (fe->id < --ep->count) {
            ep->pfds[fe->id]      = ep->pfds[ep->count];
            ep->files[fe->id]     = ep->files[ep->count];
            ep->files[fe->id]->id = fe->id;
        }
        ep->pfds[ep->count].fd      = -1;
        ep->pfds[ep->count].events  = 0;
        ep->pfds[ep->count].revents = 0;
        fe->id = -1;
        ep->event.fdcount--;
    }
    fe->mask &= ~NIO_EVENT_WRITE;
    return 0;
}

static int poll_wait(NIO_EVENT *ev, int timeout) {
    EVENT_POLL *ep = (EVENT_POLL *) ev;
    ITER  iter;
    int n, i;

#ifdef SYS_WIN
    if (ev->fdcount == 0) {
        Sleep(timeout);
        return 0;
    }
#endif
    n = poll(ep->pfds, ep->count, timeout);
#ifdef SYS_WIN
    if (n == SOCKET_ERROR) {
#else
    if (n == -1) {
#endif
        if (nio_last_error() == EVENT_EINTR) {
            return 0;
        }
        nio_msg_fatal("%s: poll error %d", __FUNCTION__, nio_last_error());
    } else if (n == 0) {
        return n;
    }

    for (i = 0; i < ep->count; i++) {
        NIO_FILE_ *fe = ep->files[i];
        nio_array_append(ep->ready, fe);
    }

    foreach(iter, ep->ready) {
        NIO_FILE_ *fe = (NIO_FILE_ *) iter.data;
        struct pollfd *pfd = &ep->pfds[fe->id];

#define EVENT_ERR	(POLLERR | POLLHUP | POLLNVAL)

        if (pfd->revents & (POLLIN | EVENT_ERR) && fe->r_proc) {
            fe->r_proc(ev, (NIO_FILE*) fe);
        }

        if (pfd->revents & (POLLOUT | EVENT_ERR ) && fe->w_proc) {
            fe->w_proc(ev, (NIO_FILE*) fe);
        }
    }

    nio_array_clean(ep->ready, NULL);
    return n;
}

static int poll_checkfd(NIO_EVENT *ev UNUSED, NIO_FILE_ *fe UNUSED) {
    return -1;
}

static nio_handle_t poll_handle(NIO_EVENT *ev) {
    (void) ev;
    return (nio_handle_t) -1;
}

static const char *poll_name(void) {
    return "poll";
}

NIO_EVENT *nio_poll_create(int size) {
    EVENT_POLL *ep = (EVENT_POLL *) mem_calloc(1, sizeof(EVENT_POLL));

    // override size with system open limit setting
    size      = nio_open_limit(0);
    if (size <= 0) {
        size = 1024;
    }
    ep->size  = size;
    ep->pfds  = (struct pollfd *) mem_calloc(size, sizeof(struct pollfd));
    ep->files = (NIO_FILE_**) mem_calloc(size, sizeof(NIO_FILE_*));
    ep->ready = nio_array_create(100);
    ep->count = 0;

    ep->event.name   = poll_name;
    ep->event.handle = poll_handle;
    ep->event.free   = poll_free;

    ep->event.event_wait = poll_wait;
    ep->event.checkfd    = (nio_event_oper *) poll_checkfd;
    ep->event.add_read   = (nio_event_oper *) poll_add_read;
    ep->event.add_write  = (nio_event_oper *) poll_add_write;
    ep->event.del_read   = (nio_event_oper *) poll_del_read;
    ep->event.del_write  = (nio_event_oper *) poll_del_write;

    return (NIO_EVENT*) ep;
}

#endif
