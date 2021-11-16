#include "stdafx.h"
#include "msg.h"
#include "iostuff.h"

#ifdef SYS_WIN
int non_blocking(socket_t fd, int on)
{
	unsigned long n = on;
	int flags = 0;

	if (ioctlsocket(fd, FIONBIO, &n) < 0) {
		msg_error("ioctlsocket(fd,FIONBIO) failed");
		return -1;
	}
	return flags;
}
#elif defined(SYS_UNIX)
# ifndef O_NONBLOCK
#  define PATTERN	FNDELAY
# else
#  define PATTERN	O_NONBLOCK
# endif

int non_blocking(socket_t fd, int on)
{
	int   flags;
	int   nonb = PATTERN;

	/*
	** NOTE: consult ALL your relevant manual pages *BEFORE* changing
	**	 these ioctl's.  There are quite a few variations on them,
	**	 as can be seen by the PCS one.  They are *NOT* all the same.
	**	 Heed this well. - Avalon.
	*/
#ifdef	NBLOCK_POSIX
	nonb |= O_NONBLOCK;
#endif
#ifdef	NBLOCK_BSD
	nonb |= O_NDELAY;
#endif

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		msg_error("%s(%d), %s: fcntl(%d, F_GETFL) error: %d",
			__FILE__, __LINE__, __FUNCTION__,
			fd, last_error());
		return -1;
	}
	if (fcntl(fd, F_SETFL, on ? flags | nonb : flags & ~nonb) < 0) {
		msg_error("%s(%d), %s: fcntl(%d, F_SETL, nonb) error: %d",
			__FILE__, __LINE__, __FUNCTION__,
			fd, last_error());
		return -1;
	}

	return flags;
}
#endif
