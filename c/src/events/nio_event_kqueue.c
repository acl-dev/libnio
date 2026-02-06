#include "stdafx.h"
#include "common.h"

#ifdef	HAS_KQUEUE

#include <dlfcn.h>
#include <sys/event.h>
#include "nio_event_kqueue.h"

typedef struct EVENT_KQUEUE {
    NIO_EVENT  event;
    int    kqfd;
    struct kevent *changes;
    int    setsize;
    int    nchanges;
    struct kevent *events;
    int    nevents;
} EVENT_KQUEUE;

static void kqueue_free(NIO_EVENT *ev) {
    EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;

    close(ek->kqfd);
    nio_mem_free(ek->changes);
    nio_mem_free(ek->events);
    nio_mem_free(ek);
}

static int kqueue_fflush(EVENT_KQUEUE *ek) {
    struct timespec ts;
    int nchanges;

    if (ek->changes == 0) {
        return 0;
    }

    ts.tv_sec  = 0;
    ts.tv_nsec = 0;
    if (kevent(ek->kqfd, ek->changes, ek->nchanges, NULL, 0, &ts) == -1) {
        nio_msg_error("%s(%d): kevent error %d, kqfd=%d",
                __FUNCTION__, __LINE__, nio_last_error(), ek->kqfd);
        return -1;
    }

    nchanges = ek->nchanges;
    ek->nchanges = 0;
    return nchanges;
}

static int kqueue_add_read(EVENT_KQUEUE *ek, NIO_FILE_ *fe) {
    struct kevent *kev;

    if (ek->nchanges >= ek->setsize) {
        if (kqueue_fflush(ek) == -1) {
            return -1;
        }
    }

    kev = &ek->changes[ek->nchanges++];
    EV_SET(kev, fe->fd, EVFILT_READ, EV_ADD, 0, 0, fe);
    if (!(fe->mask & NIO_EVENT_WRITE)) {
        ek->event.fdcount++;
    }
    fe->mask |= NIO_EVENT_READ;
    return 0;
}

static int kqueue_add_write(EVENT_KQUEUE *ek, NIO_FILE_ *fe) {
    struct kevent *kev;

    if (ek->nchanges >= ek->setsize) {
        if (kqueue_fflush(ek) == -1) {
            return -1;
        }
    }

    kev = &ek->changes[ek->nchanges++];
    EV_SET(kev, fe->fd, EVFILT_WRITE, EV_ADD, 0, 0, fe);
    if (!(fe->mask & NIO_EVENT_READ)) {
        ek->event.fdcount++;
    }
    fe->mask |= NIO_EVENT_WRITE;
    return 0;
}

static int kqueue_del_read(EVENT_KQUEUE *ek, NIO_FILE_ *fe) {
    struct kevent *kev;

    if (ek->nchanges >= ek->setsize) {
        if (kqueue_fflush(ek) == -1) {
            return -1;
        }
    }

    kev = &ek->changes[ek->nchanges++];
    EV_SET(kev, fe->fd, EVFILT_READ, EV_DELETE, 0, 0, fe);
    fe->mask &= ~NIO_EVENT_READ;
    if (!(fe->mask & NIO_EVENT_WRITE)) {
        ek->event.fdcount--;
    }
    return 0;
}

static int kqueue_del_write(EVENT_KQUEUE *ek, NIO_FILE_ *fe) {
    struct kevent *kev;

    if (ek->nchanges >= ek->setsize) {
        if (kqueue_fflush(ek) == -1) {
            return -1;
        }
    }

    kev = &ek->changes[ek->nchanges++];
    EV_SET(kev, fe->fd, EVFILT_WRITE, EV_DELETE, 0, 0, fe);
    fe->mask &= ~NIO_EVENT_WRITE;
    if (!(fe->mask & NIO_EVENT_READ)) {
        ek->event.fdcount--;
    }
    return 0;
}

static int kqueue_wait(NIO_EVENT *ev, int timeout) {
    EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;
    struct timespec ts;
    struct kevent *kev;
    NIO_FILE_ *fe;
    nio_event_proc *r_proc, *w_proc;
    int n, i;

    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;

    n = kevent(ek->kqfd, ek->changes, ek->nchanges, ek->events,
            ek->nevents, &ts);
    ek->nchanges = 0;
    nio_set_stamp(ev);

    if (n == -1) {
        if (nio_last_error() == EVENT_EINTR) {
            return 0;
        }
        nio_msg_fatal("%s: kqueue error %d: %s",
                __FUNCTION__, nio_last_error(), strerror(errno));
    } else if (n == 0) {
        return 0;
    }

    for (i = 0; i < n; i++) {
        kev = &ek->events[i];
        if (kev == NULL) {
            continue;
        }

        fe  = (NIO_FILE_ *) kev->udata;
        if (fe == NULL) {
            continue;
        }

        r_proc = fe->r_proc;
        w_proc = fe->w_proc;

        if (kev->filter == EVFILT_READ && r_proc) {
            r_proc(ev, (NIO_FILE*) fe);
        }

        if (kev->filter == EVFILT_WRITE && w_proc) {
            w_proc(ev, (NIO_FILE*) fe);
        }
    }

    return n;
}

static int kqueue_checkfd(NIO_EVENT *ev UNUSED, NIO_FILE_ *fe UNUSED) {
    return -1;
}

static long kqueue_handle(NIO_EVENT *ev) {
    EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;

    return ek->kqfd;
}

static const char *kqueue_name(void) {
    return "kqueue";
}

NIO_EVENT *nio_kqueue_create(int size) {
    EVENT_KQUEUE *ek = (EVENT_KQUEUE *) nio_mem_calloc(1, sizeof(EVENT_KQUEUE));

    if (size <= 0 || size > 1024) {
        size = 1024;
    }
    ek->changes  = (struct kevent *) nio_mem_malloc(sizeof(struct kevent) * size);
    ek->setsize  = size;
    ek->nchanges = 0;

    ek->nevents  = 100;
    ek->events   = (struct kevent *) nio_mem_malloc(sizeof(struct kevent) * ek->nevents);

    ek->kqfd     = kqueue();
    assert(ek->kqfd >= 0);

    ek->event.name   = kqueue_name;
    ek->event.handle = (nio_handle_t (*)(NIO_EVENT *)) kqueue_handle;
    ek->event.free   = kqueue_free;

    ek->event.event_fflush  = (int (*)(NIO_EVENT*)) kqueue_fflush;
    ek->event.event_wait    = kqueue_wait;
    ek->event.checkfd       = (nio_event_oper *) kqueue_checkfd;
    ek->event.add_read      = (nio_event_oper *) kqueue_add_read;
    ek->event.add_write     = (nio_event_oper *) kqueue_add_write;
    ek->event.del_read      = (nio_event_oper *) kqueue_del_read;
    ek->event.del_write     = (nio_event_oper *) kqueue_del_write;
    ek->event.del_readwrite = NULL;

    return (NIO_EVENT *) ek;
}

#endif
