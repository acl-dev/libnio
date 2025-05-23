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
#include <signal.h>

#include "nio/nio_event.h"
#include "nio/nio_iostuff.h"

static socket_t listen_addr(const char *ip, int port) {
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = PF_INET;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip);

    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(lfd, (const struct sockaddr*) &sa, sizeof(sa)) < 0) {
        close(lfd);
        return -1;
    }
    if (listen(lfd, 8192) < 0) {
        close(lfd);
        return -1;
    }
    return lfd;
}

static void read_callback(NIO_EVENT *ev, NIO_FILE *fe) {
    char buf[1024];
    int ret = read(fe->fd, buf, sizeof(buf));
    if (ret <= 0) {
        nio_event_del_readwrite(ev, fe);
        close(fe->fd);
        nio_file_free(fe);
    } else if (write(fe->fd, buf, ret) <= 0) {
        nio_event_del_readwrite(ev, fe);
        close(fe->fd);
        nio_file_free(fe);
    }
}

static void listen_callback(NIO_EVENT *ev, NIO_FILE *fe) {
    struct sockaddr_in sa;
    socklen_t len = (socklen_t) sizeof(sa);
    memset(&sa, 0, sizeof(sa));
    socket_t cfd = accept(fe->fd, (struct sockaddr*) &sa, &len);
    if (cfd == -1) {
        printf("accept error %s\r\n", strerror(errno));
    } else {
        printf("accept one fd %d\r\n", cfd);
        nio_non_blocking(cfd, 1);
        nio_tcp_nodelay(cfd, 1);
        fe = nio_file_alloc(cfd);
        if (!nio_event_add_read(ev, fe, read_callback)) {
            printf("Add event read error for fd=%d\r\n", fe->fd);
            close(cfd);
            nio_file_free(fe);
        }
    }
}

static void usage(const char *procname) {
    printf("usage: %s -s listen_ip\r\n"
           " -p listen_port\r\n"
           " -t event_type[kernel|poll|select]\r\n"
           " -m file_max[default: 102400]\r\n"
        , procname);
}

int main(int argc, char *argv[]) {
    int ch, port = 8288, event_type = NIO_EVENT_TYPE_KERNEL, file_max = 102400;
    char addr[64], event_type_s[64];

    signal(SIGPIPE, SIG_IGN);

    snprintf(addr, sizeof(addr), "127.0.0.1");

    while ((ch = getopt(argc, argv, "hs:p:t:m:")) > 0) {
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
            case 'm':
                file_max = atoi(optarg);
                break;
            default:
                break;
        }
    }

    socket_t lfd = listen_addr(addr, port);
    if (lfd == -1) {
        printf("listen %s:%d error %s\r\n", addr, port, strerror(errno));
        return 1;
    }

    printf("listen on %s:%d\r\n", addr, port);

    if (strcasecmp(event_type_s, "kernel") == 0) {
        event_type = NIO_EVENT_TYPE_KERNEL;
    } else if (strcasecmp(event_type_s, "poll") == 0) {
        event_type = NIO_EVENT_TYPE_POLL;
    } else if (strcasecmp(event_type_s, "select") == 0) {
        event_type = NIO_EVENT_TYPE_SELECT;
    }

    NIO_EVENT *ev = nio_event_create(file_max, event_type, 0);
    assert(ev);

    NIO_FILE *fe = nio_file_alloc(lfd);

    if (!nio_event_add_read(ev, fe, listen_callback)) {
        printf("Add listen to event error, fd=%d\r\n", fe->fd);
        close(fe->fd);
        nio_file_free(fe);
        return 1;
    }

    nio_event_debug(1);

    while (1) {
        nio_event_wait(ev, 1000);
    }

    return 0;
}
