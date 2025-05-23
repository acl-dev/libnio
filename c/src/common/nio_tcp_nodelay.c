#include "stdafx.h"

#include "nio_msg.h"
#include "nio/nio_iostuff.h"
#include "nio/nio_event.h"

static int getsocktype(socket_t fd)
{
	SOCK_ADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (fd == -1) {
		return -1;
	}

	if (getsockname(fd, sa, &len) == -1) {
		return -1;
	}

#ifndef	SYS_WIN
	if (sa->sa_family == AF_UNIX) {
		return AF_UNIX;
	}
#endif
	if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
		return sa->sa_family;
	}
	return -1;
}

void nio_tcp_nodelay(socket_t fd, int onoff)
{
	const char *myname = "tcp_nodelay";
	int   on = onoff ? 1 : 0;
	int   n = getsocktype(fd);

	if (n != AF_INET && n != AF_INET6) {
		return;
	}

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		(char *) &on, sizeof(on)) < 0) {

		nio_msg_error("%s(%d): set nodelay error(%d), onoff(%d)",
			myname, __LINE__, nio_last_error(), onoff);
	}
}
