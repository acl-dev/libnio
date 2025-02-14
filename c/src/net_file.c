#include "stdafx.h"
#include "common.h"

#include "event.h"

static void net_file_init(NET_FILE_ *fe, socket_t fd)
{
	net_ring_init(&fe->me);
	fe->fd     = fd;
	fe->ctx    = NULL;
	fe->id     = -1;
	fe->status = STATUS_NONE;
	fe->type   = TYPE_NONE;
	fe->oper   = 0;
	fe->mask   = 0;
	fe->r_proc = NULL;
	fe->w_proc = NULL;
#ifdef HAS_POLL
	fe->pfd    = NULL;
#endif

#ifdef HAS_IOCP
	fe->h_iocp = NULL;
	fe->reader = NULL;
	fe->writer = NULL;
	fe->iocp_sock = INVALID_SOCKET;
	memset(&fe->peer_addr, 0, sizeof(fe->peer_addr));
#endif
}

NET_FILE *net_file_alloc(socket_t fd)
{
	NET_FILE_ *fe = (NET_FILE_ *) mem_calloc(1, sizeof(NET_FILE_));
	net_file_init(fe, fd);
	return (NET_FILE*) fe;
}

socket_t net_file_fd(NET_FILE *fe)
{
	return fe->fd;
}

void net_file_free(NET_FILE *fe)
{
	mem_free((NET_FILE_*) fe);
}

void net_file_set_ctx(NET_FILE *fe, void *ctx)
{
	((NET_FILE*) fe)->ctx = ctx;
}

void *net_file_get_ctx(NET_FILE *fe)
{
	return fe->ctx;
}