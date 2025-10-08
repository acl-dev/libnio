#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "stdafx.h"
#include "common.h"

#include "nio/nio_event.h"

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef HAS_POLL
typedef struct NIO_POLLFD       NIO_POLLFD;
typedef struct NIO_POLL_EVENT   NIO_POLL_EVENT;
#endif

#ifdef HAS_EPOLL
typedef struct NIO_EPOLL_CTX    NIO_EPOLL_CTX;
typedef struct NIO_EPOLL_EVENT  NIO_EPOLL_EVENT;
#endif

#ifdef HAS_POLL
typedef void nio_poll_proc(NIO_EVENT *ev, NIO_POLL_EVENT *pe);
#endif

#ifdef HAS_EPOLL
typedef void nio_epoll_proc(NIO_EVENT *ev, NIO_EPOLL_EVENT *ee);
#endif

#ifdef HAS_IOCP
typedef struct NIO_IOCP_EVENT NIO_IOCP_EVENT;
#endif

typedef struct NIO_FILE_ NIO_FILE_;

/**
 * for each connection fd
 */
struct NIO_FILE_ {
    socket_t   fd;
    void      *ctx;

    NIO_RING   me;
    int        id;
    unsigned char status;
#define	STATUS_NONE		0
#define	STATUS_CONNECTING	(unsigned) (1 << 0)
#define	STATUS_READABLE		(unsigned) (1 << 1)
#define	STATUS_WRITABLE		(unsigned) (1 << 2)
#define	STATUS_POLLING		(unsigned) (1 << 3)

#define	SET_READABLE(x) ((x)->status |= STATUS_READABLE)
#define	SET_WRITABLE(x)	((x)->status |= STATUS_WRITABLE)
#define	SET_POLLING(x)	((x)->status |= STATUS_POLLING)

#define	CLR_READABLE(x)	((x)->status &= ~STATUS_READABLE)
#define	CLR_WRITABLE(x)	((x)->status &= ~STATUS_WRITABLE)
#define	CLR_POLLING(x)	((x)->status &= ~STATUS_POLLING)

#define	IS_READABLE(x)	((x)->status & STATUS_READABLE)
#define	IS_WRITABLE(x)	((x)->status & STATUS_WRITABLE)
#define	IS_POLLING(x)	((x)->status & STATUS_POLLING)

    unsigned char type;
#define	TYPE_NONE		0
#define	TYPE_SOCK		1
#define	TYPE_NOSOCK		2

    unsigned char oper;
#define	NIO_EVENT_ADD_READ		(unsigned) (1 << 0)
#define	NIO_EVENT_DEL_READ		(unsigned) (1 << 1)
#define	NIO_EVENT_ADD_WRITE		(unsigned) (1 << 2)
#define	NIO_EVENT_DEL_WRITE		(unsigned) (1 << 3)

    unsigned char mask;
#define	NIO_EVENT_NONE		0
#define	NIO_EVENT_READ		(unsigned) (1 << 0)
#define	NIO_EVENT_WRITE		(unsigned) (1 << 1)
#define	NIO_EVENT_ERROR		(unsigned) (1 << 2)

    nio_event_proc        *r_proc;
    nio_event_proc        *w_proc;
#ifdef HAS_POLL
    NIO_POLLFD        *pfd;
#endif
#ifdef HAS_EPOLL
    NIO_EPOLL_CTX     *epx;
#endif

#ifdef HAS_IOCP
    char              *buff;
    int                size;
    int                len;
    HANDLE             h_iocp;
    NIO_IOCP_EVENT    *reader;
    NIO_IOCP_EVENT    *writer;
    NIO_IOCP_EVENT    *poller_read;
    NIO_IOCP_EVENT    *poller_write;
    socket_t           iocp_sock;
    struct sockaddr_in peer_addr;
#endif
};

#ifdef HAS_POLL
struct NIO_POLLFD {
    NIO_FILE_      *fe;
    NIO_POLL_EVENT *pe;
    struct pollfd  *pfd;
};

struct NIO_POLL_EVENT {
    NIO_RING       me;
    nio_poll_proc *proc;
    int            nready;
    int            nfds;
    NIO_POLLFD    *fds;
};
#endif

#ifdef	HAS_EPOLL
struct NIO_EPOLL_CTX {
    int              fd;
    int              op;
    int              mask;
    int              rmask;
    NIO_FILE_       *fe;
    NIO_EPOLL_EVENT *ee;
    epoll_data_t     data;
};

struct NIO_EPOLL_EVENT {
    NIO_RING            me;
    nio_epoll_proc     *proc;
    size_t              nfds;
    NIO_EPOLL_CTX     **fds;
    int                 epfd;

    struct epoll_event *events;
    int                 maxevents;
    int                 nready;
};
#endif

typedef int  nio_event_oper(NIO_EVENT *ev, NIO_FILE_ *fe);

struct NIO_EVENT {
    NIO_RING  events;
    int       timeout;
    int       fdcount;
    size_t    setsize;
    socket_t  maxfd;
    long long stamp;

    unsigned flags;
    unsigned waiter;

    nio_handle_t (*handle)(NIO_EVENT *);

    const char *(*name)(void);
    void (*free)(NIO_EVENT *);

    int  (*event_fflush)(NIO_EVENT *);
    int  (*event_wait)(NIO_EVENT *, int);

    nio_event_oper *checkfd;
    nio_event_oper *add_read;
    nio_event_oper *add_write;
    nio_event_oper *del_read;
    nio_event_oper *del_write;
    nio_event_oper *del_readwrite;
    nio_event_oper *close_sock;
};

void nio_set_stamp(NIO_EVENT *ev);

#ifdef	__cplusplus
}
#endif

#endif  // EVENT_INCLUDE_H
