#ifndef NIO_EVENT_INCLUDE_H
#define NIO_EVENT_INCLUDE_H

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

#define NIO_EVENT_TYPE_KERNEL      0       /* epoll/kqueue/iocp    */
#define NIO_EVENT_TYPE_POLL        1       /* poll                 */
#define NIO_EVENT_TYPE_SELECT      2       /* select               */
#define NIO_EVENT_TYPE_WMSG        3       /* win message          */

typedef struct NIO_FILE         NIO_FILE;
typedef struct NIO_EVENT        NIO_EVENT;

struct NIO_FILE {
	socket_t fd;
	void *ctx;
};

typedef void (*NIO_MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);
typedef void nio_event_proc(NIO_EVENT *ev, NIO_FILE *fe);

/* nio_msg.c */
void nio_msg_register(NIO_MSG_WRITE_FN write_fn, void *ctx);
void nio_msg_unregister(void);
void nio_msg_stdout(int on);
int nio_last_error(void);

/* nio_file.c */
NIO_FILE *nio_file_alloc(socket_t fd);
void nio_file_free(NIO_FILE *fe);

socket_t nio_file_fd(NIO_FILE *fe);
void nio_file_set_ctx(NIO_FILE *fe, void *ctx);
void *nio_file_get_ctx(NIO_FILE *fe);

/* nio_event.c */
NIO_EVENT *nio_event_create(int size, int event_type, unsigned flags);
#define EVENT_F_DIRECT      (1 << 1)

const char *nio_event_name(NIO_EVENT *ev);
size_t nio_event_size(NIO_EVENT *ev);
void nio_event_free(NIO_EVENT *ev);

int  nio_event_add_read(NIO_EVENT *ev, NIO_FILE *fe, nio_event_proc *proc);
int  nio_event_add_write(NIO_EVENT *ev, NIO_FILE *fe, nio_event_proc *proc);
void nio_event_del_read(NIO_EVENT *ev, NIO_FILE *fe);
void nio_event_del_write(NIO_EVENT *ev, NIO_FILE *fe);
void nio_event_del_readwrite(NIO_EVENT *ev, NIO_FILE *fe);
int  nio_event_wait(NIO_EVENT *ev, int left);
int  nio_event_wait2(NIO_EVENT *ev, int left, void (*before_wait)(void *), void *ctx);

void nio_event_debug(int on);

#ifdef	__cplusplus
}
#endif

#endif  // NIO_EVENT_INCLUDE_H
