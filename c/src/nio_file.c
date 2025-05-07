#include "stdafx.h"
#include "common.h"

#include "event.h"

static void nio_file_init(NIO_FILE_ *fe, socket_t fd) {
    nio_ring_init(&fe->me);
    fe->fd     = fd;
    fe->ctx    = NULL;
    fe->id     = -1;
    fe->status = STATUS_NONE;
    fe->type   = TYPE_NONE;
    fe->oper   = 0;
    fe->mask   = NIO_EVENT_NONE;
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

NIO_FILE *nio_file_alloc(socket_t fd) {
    NIO_FILE_ *fe = (NIO_FILE_ *) nio_mem_calloc(1, sizeof(NIO_FILE_));
    nio_file_init(fe, fd);
    return (NIO_FILE*) fe;
}

socket_t nio_file_fd(NIO_FILE *fe) {
    return fe->fd;
}

void nio_file_free(NIO_FILE *fe) {
    nio_mem_free((NIO_FILE_*) fe);
}

void nio_file_set_ctx(NIO_FILE *fe, void *ctx) {
    ((NIO_FILE*) fe)->ctx = ctx;
}

void *nio_file_get_ctx(NIO_FILE *fe) {
    return fe->ctx;
}
