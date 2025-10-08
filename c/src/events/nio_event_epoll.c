#include "stdafx.h"
#include "common.h"

#ifdef	HAS_EPOLL

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sys/epoll.h>
#include "nio_event_epoll.h"

/****************************************************************************/

typedef struct EVENT_EPOLL {
    NIO_EVENT event;
    int   epfd;
    struct epoll_event *events;
    int   size;
} EVENT_EPOLL;

static void epoll_free(NIO_EVENT *ev) {
    EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

    close(ep->epfd);
    nio_mem_free(ep->events);
    nio_mem_free(ep);
}

static int epoll_add_read(EVENT_EPOLL *ep, NIO_FILE_ *fe) {
    struct epoll_event ee;
    int op, n;

    //if ((fe->mask & NIO_EVENT_READ)) {
    //    return 0;
    //}

    ee.events   = EPOLLIN;
    ee.data.u32 = 0;
    ee.data.u64 = 0;
    ee.data.ptr = fe;

    if ((ep->event.flags & NIO_EVENT_ONESHOT) != 0) {
        ee.events |= EPOLLONESHOT;
    }

    if (fe->mask & NIO_EVENT_WRITE) {
        ee.events |= EPOLLOUT;
        op = EPOLL_CTL_MOD;
        n  = 0;
    } else if (fe->mask & NIO_EVENT_READ) {
        op = EPOLL_CTL_MOD;
        n  = 0;
    } else {
        op = EPOLL_CTL_ADD;
        n  = 1;
    }

    if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
        fe->mask |= NIO_EVENT_READ;
        ep->event.fdcount += n;
        return 0;
    }

    if (errno != EPERM) {
        nio_msg_error("%s(%d): epoll_ctl error %d, epfd=%d, fd=%d\n",
                __FUNCTION__, __LINE__, nio_last_error(), ep->epfd, fe->fd);
    }
    return -1;
}

static int epoll_add_write(EVENT_EPOLL *ep, NIO_FILE_ *fe) {
    struct epoll_event ee;
    int op, n;

    ee.events   = EPOLLOUT;
    ee.data.u32 = 0;
    ee.data.u64 = 0;
    ee.data.ptr = fe;

    if (fe->mask & NIO_EVENT_READ) {
        ee.events |= EPOLLIN;
        op = EPOLL_CTL_MOD;
        n  = 0;
    } else if (fe->mask & NIO_EVENT_WRITE) {
        op = EPOLL_CTL_MOD;
        n  = 0;
    } else {
        op = EPOLL_CTL_ADD;
        n  = 1;
    }

    if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
        fe->mask |= NIO_EVENT_WRITE;
        ep->event.fdcount += n;
        return 0;
    }

    if (errno != EPERM) {
        nio_msg_error("%s(%d): epoll_ctl error %d, epfd=%d, fd=%d",
                __FUNCTION__, __LINE__, nio_last_error(), ep->epfd, fe->fd);
    }
    return -1;
}

static int epoll_del_read(EVENT_EPOLL *ep, NIO_FILE_ *fe) {
    struct epoll_event ee;
    int op, n = 0;

    fe->mask &= ~NIO_EVENT_READ;

    ee.events   = 0;
    ee.data.u64 = 0;
    ee.data.fd  = 0;
    ee.data.ptr = fe;

    if (fe->mask & NIO_EVENT_WRITE) {
        ee.events = EPOLLOUT;
        if (ep->event.flags & NIO_EVENT_ONESHOT) {
            ee.events |= EPOLLONESHOT;
        }

        op = EPOLL_CTL_MOD;
        n  = 0;
    } else {
        op = EPOLL_CTL_DEL;
        n  = -1;
    }

    if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
        ep->event.fdcount += n;
        return 0;
    }

    if (errno != EEXIST) {
        nio_msg_error("%s(%d), epoll_ctl error: %d, epfd=%d, fd=%d",
                __FUNCTION__, __LINE__, nio_last_error(), ep->epfd, fe->fd);
    }
    return -1;
}

static int epoll_del_write(EVENT_EPOLL *ep, NIO_FILE_ *fe) {
    struct epoll_event ee;
    int op, n;

    fe->mask &= ~NIO_EVENT_WRITE;

    ee.events   = 0;
    ee.data.u64 = 0;
    ee.data.fd  = 0;
    ee.data.ptr = fe;

    if (fe->mask & NIO_EVENT_READ) {
        ee.events = EPOLLIN;
        if (ep->event.flags & NIO_EVENT_ONESHOT) {
            ee.events |= EPOLLONESHOT;
        }

        op = EPOLL_CTL_MOD;
        n  = 0;
    } else {
        op = EPOLL_CTL_DEL;
        n  = -1;
    }

    if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
        ep->event.fdcount += n;
        return 0;
    }

    if (errno != EEXIST) {
        nio_msg_error("%s(%d), epoll_ctl error: %d, efd=%d, fd=%d",
                __FUNCTION__, __LINE__, nio_last_error(), ep->epfd, fe->fd);
    }
    return -1;
}

static int epoll_del_readwrite(EVENT_EPOLL *ep, NIO_FILE_ *fe) {
    struct epoll_event ee;
    int op;

    fe->mask = NIO_EVENT_NONE;

    ee.events   = 0;
    ee.data.u64 = 0;
    ee.data.fd  = 0;
    ee.data.ptr = fe;
    op = EPOLL_CTL_DEL;

    if (epoll_ctl(ep->epfd, op, fe->fd, &ee) == 0) {
        ep->event.fdcount--;
        return 0;
    }

    if (errno != EEXIST) {
        nio_msg_error("%s(%d), epoll_ctl error: %d, efd=%d, fd=%d",
                __FUNCTION__, __LINE__, nio_last_error(), ep->epfd, fe->fd);
    }
    return -1;
}

static int epoll_event_wait(NIO_EVENT *ev, int timeout) {
    EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
    struct epoll_event *ee;
    NIO_FILE_ *fe;
    nio_event_proc *r_proc, *w_proc;
    int n, i;

    n = epoll_wait(ep->epfd, ep->events, ep->size, timeout);
    nio_set_stamp(ev);

    if (n < 0) {
        if (nio_last_error() == EVENT_EINTR) {
            return 0;
        }
        nio_msg_fatal("%s: epoll_wait error %d: %s",
                __FUNCTION__, nio_last_error(), strerror(errno));
    } else if (n == 0) {
        return 0;
    }

    for (i = 0; i < n; i++) {
        ee = &ep->events[i];
        fe = (NIO_FILE_ *) ee->data.ptr;
        if (fe == NULL) {
            continue;
        }

        r_proc = fe->r_proc;
        w_proc = fe->w_proc;

#define EVENT_ERR	(EPOLLERR | EPOLLHUP)

        if (ee->events & (EPOLLIN | EVENT_ERR) && r_proc) {
            r_proc(ev, (NIO_FILE*) fe);
        }

        if (ee->events & (EPOLLOUT | EVENT_ERR) && w_proc) {
            w_proc(ev, (NIO_FILE*) fe);
        }
    }

    return n;
}

static int epoll_checkfd(NIO_EVENT *ev UNUSED, NIO_FILE_ *fe UNUSED) {
    if (ev->add_read(ev, fe) == -1) {
        return -1;
    }
    if (ev->del_read(ev, fe) == -1) {
        nio_msg_error("%s(%d): del_read failed, fd=%d",
                __FUNCTION__, __LINE__, fe->fd);
        return -1;
    }
    return 0;
}

static long epoll_handle(NIO_EVENT *ev) {
    EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

    return ep->epfd;
}

static const char *epoll_name(void) {
    return "epoll";
}

NIO_EVENT *nio_epoll_create(int size) {
    EVENT_EPOLL *ep = (EVENT_EPOLL *) nio_mem_calloc(1, sizeof(EVENT_EPOLL));

    ep->events = (struct epoll_event *)
        nio_mem_malloc(sizeof(struct epoll_event) * size);
    ep->size   = size;

    ep->epfd = epoll_create(1024);
    assert(ep->epfd >= 0);

    ep->event.name   = epoll_name;
    ep->event.handle = (nio_handle_t (*)(NIO_EVENT *)) epoll_handle;
    ep->event.free   = epoll_free;

    ep->event.event_wait    = epoll_event_wait;
    ep->event.checkfd       = (nio_event_oper *) epoll_checkfd;
    ep->event.add_read      = (nio_event_oper *) epoll_add_read;
    ep->event.add_write     = (nio_event_oper *) epoll_add_write;
    ep->event.del_read      = (nio_event_oper *) epoll_del_read;
    ep->event.del_write     = (nio_event_oper *) epoll_del_write;
    ep->event.del_readwrite = (nio_event_oper *) epoll_del_readwrite;

    return (NIO_EVENT*) ep;
}

#endif	// end HAS_EPOLL
