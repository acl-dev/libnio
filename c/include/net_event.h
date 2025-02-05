#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "define.h"
#include "net_ring.h"

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct NET_FILE         NET_FILE;
typedef struct NET_EVENT        NET_EVENT;

#ifdef HAS_POLL
typedef struct NET_POLLFD       NET_POLLFD;
typedef struct NET_POLL_EVENT   NET_POLL_EVENT;
#endif

#ifdef HAS_EPOLL
typedef struct NET_EPOLL_CTX    NET_EPOLL_CTX;
typedef struct NET_EPOLL_EVENT  NET_EPOLL_EVENT;
#endif

typedef int  net_event_oper(NET_EVENT *ev, NET_FILE *fe);
typedef void net_event_proc(NET_EVENT *ev, NET_FILE *fe);

#ifdef HAS_POLL
typedef void net_poll_proc(NET_EVENT *ev, NET_POLL_EVENT *pe);
#endif

#ifdef HAS_EPOLL
typedef void net_epoll_proc(NET_EVENT *ev, NET_EPOLL_EVENT *ee);
#endif

#ifdef HAS_IOCP
typedef struct NET_IOCP_EVENT NET_IOCP_EVENT;
#endif

/**
 * for each connection fd
 */
struct NET_FILE {
	NET_RING   me;
	socket_t   fd;
	void      *ctx;
	int        id;
	unsigned   status;
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

	unsigned   type;
#define	TYPE_NONE		0
#define	TYPE_SOCK		1
#define	TYPE_NOSOCK		2

	unsigned   oper;
#define	NET_EVENT_ADD_READ		(unsigned) (1 << 0)
#define	NET_EVENT_DEL_READ		(unsigned) (1 << 1)
#define	NET_EVENT_ADD_WRITE		(unsigned) (1 << 2)
#define	NET_EVENT_DEL_WRITE		(unsigned) (1 << 3)

	unsigned   mask;
#define	NET_EVENT_NONE		0
#define	NET_EVENT_READ		(unsigned) (1 << 0)
#define	NET_EVENT_WRITE		(unsigned) (1 << 1)
#define	NET_EVENT_ERROR		(unsigned) (1 << 2)

	net_event_proc        *r_proc;
	net_event_proc        *w_proc;
#ifdef HAS_POLL
	NET_POLLFD        *pfd;
#endif
#ifdef HAS_EPOLL
	NET_EPOLL_CTX     *epx;
#endif

#ifdef HAS_IOCP
	char              *buff;
	int                size;
	int                len;
	HANDLE             h_iocp;
	NET_IOCP_EVENT    *reader;
	NET_IOCP_EVENT    *writer;
	NET_IOCP_EVENT    *poller_read;
	NET_IOCP_EVENT    *poller_write;
	socket_t           iocp_sock;
	struct sockaddr_in peer_addr;
#endif
};

#ifdef HAS_POLL
struct NET_POLLFD {
	NET_FILE       *fe;
	NET_POLL_EVENT *pe;
	struct pollfd  *pfd;
};

struct NET_POLL_EVENT {
	NET_RING       me;
	net_poll_proc *proc;
	int            nready;
	int            nfds;
	NET_POLLFD    *fds;
};
#endif

#ifdef	HAS_EPOLL
struct NET_EPOLL_CTX {
	int              fd;
	int              op;
	int              mask;
	int              rmask;
	NET_FILE        *fe;
	NET_EPOLL_EVENT *ee;
	epoll_data_t     data;
};

struct NET_EPOLL_EVENT {
	NET_RING            me;
	net_epoll_proc     *proc;
	size_t              nfds;
	NET_EPOLL_CTX     **fds;
	int                 epfd;

	struct epoll_event *events;
	int                 maxevents;
	int                 nready;
};
#endif

struct NET_EVENT {
	NET_RING events;
	int      timeout;
	int      fdcount;
	ssize_t  setsize;
	socket_t maxfd;

	unsigned flag;
#define EVENT_F_IOCP (1 << 0)
#define EVENT_IS_IOCP(x) ((x)->flag & EVENT_F_IOCP)

	unsigned waiter;
	net_handle_t (*handle)(NET_EVENT *);

	const char *(*name)(void);
	void (*free)(NET_EVENT *);

	int  (*event_fflush)(NET_EVENT *);
	int  (*event_wait)(NET_EVENT *, int);

	net_event_oper *checkfd;
	net_event_oper *add_read;
	net_event_oper *add_write;
	net_event_oper *del_read;
	net_event_oper *del_write;
	net_event_oper *close_sock;
};

/* net_file.c */
void net_file_init(NET_FILE *fe, socket_t fd);
NET_FILE *net_file_alloc(socket_t fd);
void net_file_free(NET_FILE *fe);

/* net_event.c */
NET_EVENT *net_event_create(int size, int event_type);
const char *net_event_name(NET_EVENT *ev);
ssize_t net_event_size(NET_EVENT *ev);
void net_event_free(NET_EVENT *ev);
void net_event_close(NET_EVENT *ev, NET_FILE *fe);

int  net_event_add_read(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc);
int  net_event_add_write(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc);
void net_event_del_read(NET_EVENT *ev, NET_FILE *fe);
void net_event_del_write(NET_EVENT *ev, NET_FILE *fe);
int  net_event_wait(NET_EVENT *ev, int left);

#ifdef	__cplusplus
}
#endif

#endif  // EVENT_INCLUDE_H
