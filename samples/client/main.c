#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#include "net_event/net_event.h"
#include "net_event/net_iostuff.h"

typedef struct {
    int max_loop;
    int cocurrent;
    int finished;
    int stop;
    int count;
} gio_ctx_t;

typedef struct {
    gio_ctx_t *gctx;
} io_ctx_t;

static double stamp_sub(const struct timeval *from, const struct timeval *sub) {
    struct timeval res;

    memcpy(&res, from, sizeof(struct timeval));

    res.tv_usec -= sub->tv_usec;
    if (res.tv_usec < 0) {
        --res.tv_sec;
        res.tv_usec += 1000000;
    }

    res.tv_sec -= sub->tv_sec;
    return res.tv_sec * 1000.0 + res.tv_usec / 1000.0;
}

static socket_t connect_server(const char *ip, int port) {
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = PF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    net_non_blocking(fd, 1);
    net_tcp_nodelay(fd, 0);

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (connect(fd, (const struct sockaddr*) &sa, sizeof(sa)) < 0) {
        if (errno != EINPROGRESS && errno != EISCONN) {
            close(fd);
            return -1;
        }
    }

    return fd;
}

static void close_connection(NET_EVENT *ev, NET_FILE *fe) {
    io_ctx_t *ctx = (io_ctx_t*) fe->ctx;
    net_event_close(ev, fe);
    close(fe->fd);
    net_file_free(fe);

    if (++ctx->gctx->finished >= ctx->gctx->cocurrent) {
        ctx->gctx->stop = 1;
        printf("All connections closed, finished=%d, cocurrent=%d\r\n",
               ctx->gctx->finished, ctx->gctx->cocurrent);
    }

    free(ctx);
}

static void read_callback(NET_EVENT *ev, NET_FILE *fe) {
    io_ctx_t *ctx = (io_ctx_t*) fe->ctx;
    char buf[1024];
    int ret = read(fe->fd, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        close_connection(ev, fe);
    } else if (++ctx->gctx->count >= ctx->gctx->max_loop) {
        printf("All over, stop now, count=%d, max=%d!\r\n",
               ctx->gctx->count, ctx->gctx->max_loop);
        ctx->gctx->stop = 1;
    } else if (write(fe->fd, buf, ret) <= 0) {
        close_connection(ev, fe);
    } else if (ctx->gctx->count <= 10) {
        buf[ret] = 0;
        printf("%s", buf);
    }
}

static void connect_callback(NET_EVENT *ev, NET_FILE *fe) {
    net_event_del_write(ev, fe);

    const char *s = "hello world!\r\n";
    if (write(fe->fd, s, strlen(s)) == -1) {
        close_connection(ev, fe);
    } else if (!net_event_add_read(ev, fe, read_callback)) {
        printf("Add event read error, fd=%d\r\n", fe->fd);
        close_connection(ev, fe);
    }
}

static void usage(const char *procname) {
    printf("usage: %s -s server_ip\r\n"
           " -p listen_port\r\n"
           " -t event_type[kernel|poll|select]\r\n"
           " -c cocurrent\r\n"
           " -n max_loop\r\n"
           " -m file_max[default: 102400]\r\n"
           " -d us_delay_when_connect[default: 0]\r\n"
        , procname);
}

int main(int argc, char *argv[]) {
    int ch, port = 8288, event_type = NET_EVENT_TYPE_KERNEL;
    int cocurrent = 10, max_loop = 100, file_max = 102400, delay = 0;
    char addr[64], event_type_s[64];

    signal(SIGPIPE, SIG_IGN);
    snprintf(addr, sizeof(addr), "127.0.0.1");

    while ((ch = getopt(argc, argv, "hs:p:t:c:n:m:d:")) > 0) {
        switch (ch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 's':
                snprintf(addr, sizeof(addr), "%s", optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                snprintf(event_type_s, sizeof(event_type_s), "%s", optarg);
                break;
            case 'c':
                cocurrent = atoi(optarg);
                break;
            case 'n':
                max_loop = atoi(optarg);
                break;
            case 'm':
                file_max = atoi(optarg);
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            default:
                break;
        }
    }

    if (strcasecmp(event_type_s, "kernel") == 0) {
        event_type = NET_EVENT_TYPE_KERNEL;
    } else if (strcasecmp(event_type_s, "poll") == 0) {
        event_type = NET_EVENT_TYPE_POLL;
    } else if (strcasecmp(event_type_s, "select") == 0) {
        event_type = NET_EVENT_TYPE_SELECT;
    }

    NET_EVENT *ev = net_event_create(file_max, event_type);
    assert(ev);

    gio_ctx_t *gctx = calloc(1, sizeof(gio_ctx_t));
    gctx->max_loop = max_loop;
    gctx->cocurrent = cocurrent;
    gctx->finished  = 0;

    int i;
    for (i = 0; i < cocurrent; i++) {
        socket_t fd = connect_server(addr, port);
        if (fd == -1) {
            printf("connect %s:%d error %s\r\n", addr, port, strerror(errno));
            break;
        }

        NET_FILE *fe = net_file_alloc(fd);
        io_ctx_t *ctx = calloc(1, sizeof(io_ctx_t));
        ctx->gctx = gctx;
        fe->ctx   = ctx;
        if (!net_event_add_write(ev, fe, connect_callback)) {
            printf("Add one fd=%d error\r\n", fd);
            close(fd);
            net_file_free(fe);
            break;
        }

        if (delay > 0) {
            usleep(delay);
        }
    }

    struct timeval begin;
    gettimeofday(&begin, NULL);

    while (!gctx->stop) {
        net_event_wait(ev, 1000);
    }

    struct timeval end;
    gettimeofday(&end, NULL);

    double cost = stamp_sub(&end, &begin);
    double speed = ((long long) gctx->count * 1000) / (cost >= 0.01 ? cost : 0.01);
    printf("All over, count=%d, cost=%.2f ms, speed=%.2f\r\n",
           gctx->count, cost, speed);
    free(gctx);
    return 0;
}
