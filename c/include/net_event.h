#ifndef NET_EVENT_INCLUDE_H
#define NET_EVENT_INCLUDE_H

#include <stdlib.h>

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef USE_SOCKET_T
#define USE_SOCKET_T

# if defined(_WIN32) || defined (_WIN64)
typedef SOCKET socket_t;
typedef int socklen_t;
# else
typedef int socket_t;
# endif
#endif

#define NET_EVENT_TYPE_KERNEL      0       /* epoll/kqueue/iocp    */
#define NET_EVENT_TYPE_POLL        1       /* poll                 */
#define NET_EVENT_TYPE_SELECT      2       /* select               */
#define NET_EVENT_TYPE_WMSG        3       /* win message          */

typedef struct NET_FILE         NET_FILE;
typedef struct NET_EVENT        NET_EVENT;

struct NET_FILE {
	socket_t fd;
	void *ctx;
};

typedef void net_event_proc(NET_EVENT *ev, NET_FILE *fe);

/* net_file.c */
NET_FILE *net_file_alloc(socket_t fd);
void net_file_free(NET_FILE *fe);

socket_t net_file_fd(NET_FILE *fe);
void net_file_set_ctx(NET_FILE *fe, void *ctx);
void *net_file_get_ctx(NET_FILE *fe);

/* net_event.c */
NET_EVENT *net_event_create(int size, int event_type);
const char *net_event_name(NET_EVENT *ev);
size_t net_event_size(NET_EVENT *ev);
void net_event_free(NET_EVENT *ev);
void net_event_close(NET_EVENT *ev, NET_FILE *fe);

int  net_event_add_read(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc);
int  net_event_add_write(NET_EVENT *ev, NET_FILE *fe, net_event_proc *proc);
void net_event_del_read(NET_EVENT *ev, NET_FILE *fe);
void net_event_del_write(NET_EVENT *ev, NET_FILE *fe);
int  net_event_wait(NET_EVENT *ev, int left);

void net_event_debug(int on);

#ifdef	__cplusplus
}
#endif

#endif  // NET_EVENT_INCLUDE_H
