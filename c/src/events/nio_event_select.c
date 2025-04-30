#include "stdafx.h"

#ifdef HAS_SELECT

#include "nio/nio_iostuff.h"
#include "common.h"
#include "nio_event_select.h"

typedef struct EVENT_SELECT {
    NIO_EVENT  event;
    fd_set rset;
    fd_set wset;
    fd_set xset;
    NIO_FILE_ **files;
    int    size;
    int    count;
    socket_t maxfd;
    int    dirty;
    NIO_ARRAY *ready;
} EVENT_SELECT;

static void select_free(NIO_EVENT *ev) {
    EVENT_SELECT *es = (EVENT_SELECT *) ev;
    nio_mem_free(es->files);
    nio_array_free(es->ready, NULL);
    nio_mem_free(es);
}

static int select_add_read(EVENT_SELECT *es, NIO_FILE_ *fe) {
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

    fe->mask |= NIO_EVENT_READ;
    FD_SET(fe->fd, &es->rset);
    return 0;
}

static int select_add_write(EVENT_SELECT *es, NIO_FILE_ *fe) {
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

    fe->mask |= NIO_EVENT_WRITE;
    FD_SET(fe->fd, &es->wset);
    return 0;
}

static int select_del_read(EVENT_SELECT *es, NIO_FILE_ *fe) {
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
    fe->mask &= ~NIO_EVENT_READ;
    return 0;
}

static int select_del_write(EVENT_SELECT *es, NIO_FILE_ *fe) {
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
    fe->mask &= ~NIO_EVENT_WRITE;
    return 0;
}

static int select_event_wait(NIO_EVENT *ev, int timeout) {
    EVENT_SELECT *es = (EVENT_SELECT *) ev;
    fd_set rset = es->rset, wset = es->wset, xset = es->xset;
    struct timeval tv, *tp;
    nio_event_proc *r_proc, *w_proc;
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
            NIO_FILE_ *fe = es->files[i];
            if (fe->fd > es->maxfd) {
                es->maxfd = fe->fd;
            }
        }
    }
    n = select(es->maxfd + 1, &rset, 0, &xset, tp);
#endif
    if (n < 0) {
        if (nio_last_error() == EVENT_EINTR) {
            return 0;
        }
        nio_msg_fatal("%s: select error %d", __FUNCTION__, nio_last_error());
    } else if (n == 0) {
        return 0;
    }

    for (i = 0; i < es->count; i++) {
        NIO_FILE_ *fe = es->files[i];
        nio_array_append(es->ready, fe);
    }

    foreach(iter, es->ready) {
        NIO_FILE_ *fe = (NIO_FILE_ *) iter.data;

        r_proc = fe->r_proc;
        w_proc = fe->w_proc;

        if (FD_ISSET(fe->fd, &xset)) {
            if (FD_ISSET(fe->fd, &es->rset) && r_proc) {
                r_proc(ev, (NIO_FILE*) fe);
            }
            if (FD_ISSET(fe->fd, &es->wset) && w_proc) {
                w_proc(ev, (NIO_FILE*) fe);
            }
        } else {
            if (FD_ISSET(fe->fd, &rset) && r_proc) {
                r_proc(ev, (NIO_FILE*) fe);
            }
            if (FD_ISSET(fe->fd, &wset) && w_proc) {
                w_proc(ev, (NIO_FILE*) fe);
            }
        }
    }

    nio_array_clean(es->ready, NULL);
    return n;
}

static int select_checkfd(NIO_EVENT *ev UNUSED, NIO_FILE_ *fe UNUSED) {
    return -1;
}

static nio_handle_t select_handle(NIO_EVENT *ev) {
    (void) ev;
    return (nio_handle_t)-1;
}

static const char *select_name(void) {
    return "select";
}

NIO_EVENT *nio_select_create(int size) {
    EVENT_SELECT *es = (EVENT_SELECT *) nio_mem_calloc(1, sizeof(EVENT_SELECT));

    // override size with system open limit setting
    size      = nio_open_limit(0);
    if (size <= 0) {
        size = 1024;
    }

    es->maxfd = -1;
    es->dirty = 0;
    es->files = (NIO_FILE_**) nio_mem_calloc(size, sizeof(NIO_FILE_*));
    es->size  = size;
    es->ready = nio_array_create(100);
    es->count = 0;
    FD_ZERO(&es->rset);
    FD_ZERO(&es->wset);
    FD_ZERO(&es->xset);

    es->event.name   = select_name;
    es->event.handle = select_handle;
    es->event.free   = select_free;

    es->event.event_wait = select_event_wait;
    es->event.checkfd    = (nio_event_oper *) select_checkfd;
    es->event.add_read   = (nio_event_oper *) select_add_read;
    es->event.add_write  = (nio_event_oper *) select_add_write;
    es->event.del_read   = (nio_event_oper *) select_del_read;
    es->event.del_write  = (nio_event_oper *) select_del_write;

    return (NIO_EVENT*) es;
}

#endif
