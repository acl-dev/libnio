#include "stdafx.h"
#include "common.h"

#ifdef HAS_IOCP

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Mswsock.lib")

#include "nio_event_iocp.h"

typedef BOOL (PASCAL FAR* LPFN_CONNECTEX) (
    IN   SOCKET s,
    IN   const struct sockaddr FAR *name,
    IN   int namelen,
    IN   PVOID lpSendBuffer OPTIONAL,
    IN   DWORD dwSendDataLength,
    OUT  LPDWORD lpdwBytesSent,
    IN   LPOVERLAPPED lpOverlapped
);

typedef struct EVENT_IOCP {
    NIO_EVENT  event;
    NIO_FILE_ **files;
    int    size;
    int    count;
    HANDLE h_iocp;
    ARRAY *events;
} EVENT_IOCP;

struct IOCP_EVENT {
    OVERLAPPED overlapped;
    int   type;
#define	IOCP_NIO_EVENT_READ		(1 << 0)
#define IOCP_NIO_EVENT_WRITE	(1 << 2)
#define IOCP_EVENT_DEAD		(1 << 3)
#define	IOCP_EVENT_POLLR	(1 << 4)
#define	IOCP_EVENT_POLLW	(1 << 4)
    int   refer;
    NIO_FILE_ *fe;
    event_proc *proc;

#define ACCEPT_ADDRESS_LENGTH ((sizeof(struct sockaddr_in) + 16))
    char  myAddrBlock[ACCEPT_ADDRESS_LENGTH * 2];
};

static void iocp_remove(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    if (fe->id < --ev->count) {
        ev->files[fe->id]     = ev->files[ev->count];
        ev->files[fe->id]->id = fe->id;
    }

    fe->id = -1;
    ev->event.fdcount--;
}

static int iocp_close_sock(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    if (fe->h_iocp != NULL) {
        fe->h_iocp = NULL;
    }

    if (fe->id >= 0) {
        iocp_remove(ev, fe);
    }

    /* must close socket before releasing fe->reader/fe->writer */
    if (fe->fd != INVALID_SOCKET) {
        closesocket(fe->fd);

        /* set fd INVALID_SOCKET notifying the caller the socket be closed*/
        fe->fd = INVALID_SOCKET;
    }

    /* On Windows XP, must check if the OVERLAPPED IO is in STATUS_PENDING
     * status before the socket being closed.
     */

    if (fe->reader) {
        /*
         * If the IOCP Port isn't in completed status, the OVERLAPPED
         * object should not be released, which should be released in
         * the GetQueuedCompletionStatus process.
         */
        if (HasOverlappedIoCompleted(&fe->reader->overlapped)) {
            if (fe->reader->refer == 0) {
                nio_mem_free(fe->reader);
            } else {
                fe->reader->fe = NULL;
            }
        } else {
            fe->reader->type = IOCP_EVENT_DEAD;
            fe->reader->fe   = NULL;
        }
        fe->reader = NULL;
    }

    if (fe->writer) {
        /* If the IOCP Port is in incompleted status, the OVERLAPPED
         * object shouldn't be released, which should be released in
         * the GetQueuedCompletionStatus process.
         */
        if (HasOverlappedIoCompleted(&fe->writer->overlapped)) {
            if (fe->writer->refer == 0) {
                nio_mem_free(fe->writer);
            } else {
                fe->writer->fe = NULL;
            }
        } else {
            fe->writer->type = IOCP_EVENT_DEAD;
            fe->writer->fe   = NULL;
        }

        fe->writer = NULL;
    }

    if (fe->poller_read) {
        if (fe->poller_read->refer == 0) {
            nio_mem_free(fe->poller_read);
        } else {
            fe->poller_read->fe = NULL;
        }
    }

    if (fe->poller_write) {
        if (fe->poller_write->refer == 0) {
            nio_mem_free(fe->poller_write);
        } else {
            fe->poller_write->fe = NULL;
        }
    }

    return 1;
}

static void iocp_check(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    if (fe->id == -1) {
        assert(ev->count < ev->size);
        fe->id = ev->count++;
        ev->files[fe->id] = fe;
        ev->event.fdcount++;
    } else {
        assert(fe->id >= 0 && fe->id < ev->count);
        assert(ev->files[fe->id] == fe);
    }

    if (fe->h_iocp == NULL) {
        fe->h_iocp = CreateIoCompletionPort((HANDLE) fe->fd,
                ev->h_iocp, (ULONG_PTR) fe, 0);
        if (fe->h_iocp != ev->h_iocp) {
            nio_msg_fatal("%s(%d): CreateIoCompletionPort error(%s)",
                    __FUNCTION__, __LINE__, nio_last_serror());
        }
    }
}

static int iocp_add_listen(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    DWORD    ReceiveLen = 0;
    socket_t sock;
    BOOL     ret;

    sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
            0, 0, WSA_FLAG_OVERLAPPED);

    fe->iocp_sock = sock;
    ret = AcceptEx(fe->fd,
            sock,
            fe->reader->myAddrBlock,
            0,
            ACCEPT_ADDRESS_LENGTH,
            ACCEPT_ADDRESS_LENGTH,
            &ReceiveLen,
            &fe->reader->overlapped);

    if (ret == TRUE) {
        fe->mask |= NIO_EVENT_READ;
        return 1;
    } else if (last_error() == ERROR_IO_PENDING) {
        fe->mask |= NIO_EVENT_READ;
        return 0;
    } else {
        nio_msg_warn("%s(%d): AcceptEx error(%s)",
                __FUNCTION__, __LINE__, nio_last_serror());
        fe->mask |= EVENT_ERROR;
        assert(fe->reader);
        nio_array_append(ev->events, fe->reader);
        return 1;
    }
}

static int iocp_add_read(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    int ret;
    WSABUF wsaData;
    DWORD  flags = 0, len = 0;
    IOCP_EVENT *event;
    int is_listener = is_listen_socket(fe->fd);

    iocp_check(ev, fe);

    /* Check if the fe has been set STATUS_POLLING in io.c/poll.c/socket.c,
     * and will set poller_write or writer IOCP_EVENT.
     */
    if (IS_POLLING(fe)) {
        if (fe->poller_read == NULL) {
            fe->poller_read = (IOCP_EVENT*) nio_mem_calloc(1, sizeof(IOCP_EVENT));
            fe->poller_read->refer = 0;
            fe->poller_read->fe    = fe;
            fe->poller_read->type  = IOCP_EVENT_POLLR;
        }
        event = fe->poller_read;
    } else {
        if (fe->reader == NULL) {
            fe->reader = (IOCP_EVENT*) nio_mem_calloc(1, sizeof(IOCP_EVENT));
            fe->reader->refer = 0;
            fe->reader->fe    = fe;
            fe->reader->type = IOCP_NIO_EVENT_READ;
        }
        event = fe->reader;
    }

    event->proc = fe->r_proc;
    event->refer++;

    if (is_listener) {
        return iocp_add_listen(ev, fe);
    }

    wsaData.buf = fe->buff;
    wsaData.len = fe->size;

    ret = WSARecv(fe->fd, &wsaData, 1, &len, &flags,
            (OVERLAPPED*) &event->overlapped, NULL);

    fe->len = (int) len;

    if (ret != SOCKET_ERROR) {
        fe->mask |= NIO_EVENT_READ;
        return 1;
    } else if (last_error() == ERROR_IO_PENDING) {
        fe->mask |= NIO_EVENT_READ;
        return 0;
    } else {
        nio_msg_warn("%s(%d): ReadFile error(%d), fd=%d",
                __FUNCTION__, __LINE__, last_error(), fe->fd);
        fe->mask |= EVENT_ERROR;
        assert(fe->reader);
        nio_array_append(ev->events, fe->reader);
        return -1;
    }
}

static int iocp_add_write(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    DWORD sendBytes;
    BOOL  ret;
    IOCP_EVENT *event;

    iocp_check(ev, fe);

    /* Check if the fe has been set STATUS_POLLING in io.c/poll.c/socket.c,
     * and will set poller_write or writer IOCP_EVENT.
     */
    if (IS_POLLING(fe)) {
        if (fe->poller_write == NULL) {
            fe->poller_write = (IOCP_EVENT*) nio_mem_calloc(1, sizeof(IOCP_EVENT));
            fe->poller_write->refer = 0;
            fe->poller_write->fe    = fe;
            fe->poller_write->type  = IOCP_EVENT_POLLW;
        }
        event = fe->poller_write;
    } else {
        if (fe->writer == NULL) {
            fe->writer        = (IOCP_EVENT*) nio_mem_malloc(sizeof(IOCP_EVENT));
            fe->writer->refer = 0;
            fe->writer->fe    = fe;
            fe->writer->type = IOCP_NIO_EVENT_WRITE;
        }
        event = fe->writer;
    }

    event->proc = fe->w_proc;
    event->refer++;

    if (fe->status & STATUS_CONNECTING) {
        //return iocp_add_connect(ev, fe);
    }

    ret = WriteFile((HANDLE) fe->fd, NULL, 0, &sendBytes,
            &event->overlapped);

    if (ret == TRUE) {
        fe->mask |= NIO_EVENT_WRITE;
        return 0;
    } else if (last_error() != ERROR_IO_PENDING) {
        fe->mask |= NIO_EVENT_WRITE;
        return 0;
    } else {
        nio_msg_warn("%s(%d): WriteFile error(%s)",
                __FUNCTION__, __LINE__, nio_last_serror());
        fe->mask |= EVENT_ERROR;
        assert(fe->writer);
        nio_array_append(ev->events, fe->writer);
        return -1;
    }
}

static int iocp_del_read(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    if (!(fe->mask & NIO_EVENT_READ)) {
        return 0;
    }

    assert(fe->id >= 0 && fe->id < ev->count);
    fe->mask &= ~NIO_EVENT_READ;

    if (fe->reader) {
        fe->reader->type &= ~IOCP_NIO_EVENT_READ;
    }
    if (fe->poller_read) {
        fe->poller_read->type &= ~IOCP_EVENT_POLLR;
    }

    if (fe->mask == 0) {
        iocp_remove(ev, fe);
    }
    return 0;
}

static int iocp_del_write(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    if (!(fe->mask & NIO_EVENT_WRITE)) {
        return 0;
    }

    assert(fe->id >= 0 && fe->id < ev->count);
    fe->mask &= ~NIO_EVENT_WRITE;

    if (fe->writer) {
        fe->writer->type &= ~IOCP_NIO_EVENT_WRITE;
    }
    if (fe->poller_write) {
        fe->poller_write->type &= ~IOCP_EVENT_POLLW;
    }

    if (fe->mask == 0) {
        iocp_remove(ev, fe);
    }
    return 0;
}

static void iocp_event_save(EVENT_IOCP *ei, IOCP_EVENT *event,
        NIO_FILE_ *fe, DWORD trans) {
    if ((event->type & (IOCP_NIO_EVENT_READ | IOCP_EVENT_POLLR))) {
        fe->mask &= ~NIO_EVENT_READ;
    } else if ((event->type & (IOCP_NIO_EVENT_WRITE | IOCP_EVENT_POLLW))) {
        fe->mask &= ~NIO_EVENT_WRITE;
    }

    fe->len = (int) trans;
    nio_array_append(ei->events, event);
}

static int iocp_wait(NIO_EVENT *ev, int timeout) {
    EVENT_IOCP *ei = (EVENT_IOCP *) ev;
    IOCP_EVENT *event;

    for (;;) {
        DWORD bytesTransferred;
        NIO_FILE_ *fe;
        event = NULL;

        BOOL isSuccess = GetQueuedCompletionStatus(ei->h_iocp,
                &bytesTransferred, (PULONG_PTR) &fe,
                (OVERLAPPED**) &event, timeout);

        if (!isSuccess) {
            if (event == NULL) {
                break;
            }

            if (event->type & IOCP_EVENT_DEAD) {
                nio_mem_free(event);
                continue;
            }

            assert(fe);
            iocp_event_save(ei, event, fe, bytesTransferred);
            continue;
        }

        event->refer--;
        if (event->fe == NULL) {
            if (event->refer == 0) {
                nio_mem_free(event);
            }
            continue;
        }

        if (fe != event->fe) {
            assert(fe == event->fe);
        }

        if (fe->mask & EVENT_ERROR) {
            continue;
        }

        iocp_event_save(ei, event, fe, bytesTransferred);
        timeout = 0;
    }

    /* peek and handle all IOCP NIO_EVENT added in iocp_event_save(). */
    while ((event = (IOCP_EVENT*) ei->events->pop_back(ei->events)) != NULL) {
        if (event->proc && event->fe) {
            event->proc(ev, ((NIO_FILE*) event->fe);
        }
    }

    return 0;
}

static void iocp_free(NIO_EVENT *ev) {
    EVENT_IOCP *ei = (EVENT_IOCP *) ev;

    if (ei->h_iocp) {
        CloseHandle(ei->h_iocp);
    }
    nio_array_free(ei->events, NULL);
    nio_mem_free(ei->files);
    nio_mem_free(ei);
}

static int iocp_checkfd(EVENT_IOCP *ev, NIO_FILE_ *fe) {
    (void) ev;
    return getsocktype(fe->fd) == -1 ? -1 : 0;
}

static nio_handle_t iocp_handle(NIO_EVENT *ev) {
    EVENT_IOCP *ei = (EVENT_IOCP *) ev;
    return (nio_handle_t) ei->h_iocp;
}

static const char *iocp_name(void) {
    return "iocp";
}

NIO_EVENT *nio_iocp_create(int size) {
    EVENT_IOCP *ei = (EVENT_IOCP *) nio_mem_calloc(1, sizeof(EVENT_IOCP));

    ei->h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (ei->h_iocp == NULL) {
        nio_msg_fatal("%s(%d): create iocp error(%s)",
                __FUNCTION__, __LINE__, nio_last_serror());
    }

    ei->events = nio_array_create(100);

    ei->files = (NIO_FILE_**) nio_mem_calloc(size, sizeof(NIO_FILE_*));
    ei->size  = size;
    ei->count = 0;

    ei->event.name   = iocp_name;
    ei->event.handle = iocp_handle;
    ei->event.free   = iocp_free;
    ei->event.flags  = EVENT_F_IOCP;

    ei->event.event_wait     = iocp_wait;
    ei->event.checkfd        = (nio_event_oper *) iocp_checkfd;
    ei->event.add_read       = (nio_event_oper *) iocp_add_read;
    ei->event.add_write      = (nio_event_oper *) iocp_add_write;
    ei->event.del_read       = (nio_event_oper *) iocp_del_read;
    ei->event.del_write      = (nio_event_oper *) iocp_del_write;
    ei->event.del_readwrite  = NULL;
    ei->event.close_sock     = (nio_event_oper *) iocp_close_sock;

    return (NIO_EVENT *) ei;
}

#endif
