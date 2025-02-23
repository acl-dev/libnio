#ifndef	__NIO_DEFINE_INCLUDE_H__
#define	__NIO_DEFINE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(__linux__)
# define LINUX
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_EPOLL
#elif defined(__FreeBSD__)
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_KQUEUE
#elif defined(__APPLE__)
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_KQUEUE
# define _XOPEN_SOURCE
#elif defined(_WIN32) || defined(_WIN64)

# if(_MSC_VER >= 1300)
#  undef FD_SETSIZE
#  define FD_SETSIZE 10240
#  include <winsock2.h>
#  include <mswsock.h>
# else
#  include <winsock.h>
# endif

# if _MSC_VER >= 1500
#  include <netioapi.h>
# endif

# include <ws2tcpip.h> /* for getaddrinfo */
# include <process.h>
# include <stdint.h>

# define SYS_WIN
# define HAS_SELECT
# if(_WIN32_WINNT >= 0x0600)
#  define HAS_POLL
# endif
# define HAS_WMSG
# define HAS_IOCP
# define __thread __declspec(thread)

typedef unsigned long nfds_t;

#else
# error "unknown OS"
#endif

#ifdef SYS_UNIX
# ifndef WINAPI
#  define WINAPI
# endif
#endif

#if defined(_WIN32) || defined (_WIN64)
# include <winsock2.h>

/* typedef intptr_t ssize_t; */
# ifndef	HAS_SSIZE_T
#  define	HAS_SSIZE_T
/* typedef intptr_t ssize_t; */
#  if defined(_WIN64)
typedef __int64 ssize_t;
#  elif defined(_WIN32)
typedef int ssize_t;
#  else
typedef long ssize_t;
#  endif
# endif
//typedef SOCKET socket_t;
//typedef int socklen_t;

# define	EVENT_ETIMEDOUT		WSAETIMEDOUT
# define	EVENT_ENOMEM		WSAENOBUFS
# define	EVENT_EINVAL		WSAEINVAL
# define	EVENT_ECONNREFUSED	WSAECONNREFUSED
# define	EVENT_ECONNRESET	WSAECONNRESET
# define	EVENT_EHOSTDOWN		WSAEHOSTDOWN
# define	EVENT_EHOSTUNREACH	WSAEHOSTUNREACH
# define	EVENT_EINTR		WSAEINTR
# define	EVENT_ENETDOWN		WSAENETDOWN
# define	EVENT_ENETUNREACH	WSAENETUNREACH
# define	EVENT_ENOTCONN		WSAENOTCONN
# define	EVENT_EISCONN		WSAEISCONN
# define	EVENT_EWOULDBLOCK	WSAEWOULDBLOCK
# define	EVENT_EAGAIN		EVENT_EWOULDBLOCK	/* xxx */
# define	EVENT_ENOBUFS		WSAENOBUFS
# define	EVENT_ECONNABORTED	WSAECONNABORTED
# define	EVENT_EINPROGRESS	WSAEINPROGRESS

#else

# define INVALID_SOCKET	-1
//typedef int socket_t;

# define	EVENT_ETIMEDOUT		ETIMEDOUT
# define	EVENT_ENOMEM		ENOMEM
# define	EVENT_EINVAL		EINVAL
# define	EVENT_ECONNREFUSED	ECONNREFUSED
# define	EVENT_ECONNRESET	ECONNRESET
# define	EVENT_EHOSTDOWN		EHOSTDOWN
# define	EVENT_EHOSTUNREACH	EHOSTUNREACH
# define	EVENT_EINTR			EINTR
# define	EVENT_EAGAIN		EAGAIN
# define	EVENT_ENETDOWN		ENETDOWN
# define	EVENT_ENETUNREACH	ENETUNREACH
# define	EVENT_ENOTCONN		ENOTCONN
# define	EVENT_EISCONN		EISCONN
# define	EVENT_EWOULDBLOCK	EWOULDBLOCK
# define	EVENT_ENOBUFS		ENOBUFS
# define	EVENT_ECONNABORTED	ECONNABORTED
# define	EVENT_EINPROGRESS	EINPROGRESS

#endif

# include <stdint.h>
typedef intptr_t nio_handle_t;

#ifndef event_unused
# ifdef	__GNUC__
#  define event_unused	__attribute__ ((__unused__))
# else
#  define event_unused  /* Ignore */
# endif
#endif

#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define	PRINTF(format_idx, arg_idx) \
	__attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define	SCANF(format_idx, arg_idx) \
	__attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define	NORETURN __attribute__((__noreturn__))
#define	UNUSED __attribute__((__unused__))
#else
#define	PRINTF(format_idx, arg_idx)
#define	SCANF
#define	NORETURN
#define	UNUSED
#endif	/* __GNUC__ */

#if	__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define	DEPRECATED __attribute__((__deprecated__))
#elif	defined(_MSC_VER) && (_MSC_VER >= 1300)
#define	DEPRECATED __declspec(deprecated)
#else
#define	DEPRECATED
#endif	/* __GNUC__ */

#if	__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define	DEPRECATED_FOR(f) __attribute__((deprecated("Use " #f " instead")))
#elif	defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define	DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define	DEPRECATED_FOR(f)	DEPRECATED
#endif	/* __GNUC__ */

#ifdef	__cplusplus
}
#endif

#endif /* __NIO_DEFINE_INCLUDE_H__ */
